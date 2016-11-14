#pragma once
// author: Dmitry Lysachenko
// date: Saturday 20 april 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <cstdint>
#include <memory>
#include <atomic>
#include <utility>
#include <string>
#include <chrono>
#include <system_error>

#include <boost/config.hpp>
#include <ext/iostreams/socket_streambuf_base.hpp>
#include <ext/iostreams/bsdsock_config.hpp>

namespace ext
{
	typedef addrinfo addrinfo_type;
	typedef sockaddr sockaddr_type;
	typedef int      socket_handle_type;

	struct addrinfo_deleter
	{
		void operator()(addrinfo_type * ptr) const;
	};

	typedef std::unique_ptr<addrinfo_type, addrinfo_deleter> addrinfo_ptr;


	/// производит инициализацию библиотек необходимых для использования bsdsock_streambuf.
	/// это, если включен, OpenSSL. по факту ext::openssl_init()
	void bsdsock_stream_init();

	/// error category for network address and service translation.
	/// EAI_* codes, gai_strerror
	const std::error_category & gai_error_category();
	BOOST_NORETURN void throw_bsdsock_error(int code, const char * errmsg);
	BOOST_NORETURN void throw_bsdsock_error(int code, const std::string & errmsg);

	/// ::inet_ntop wrapper, все строки в utf8
	/// @Throws std::system_error в случае системной ошибки
	void inet_ntop(const sockaddr * addr, std::string & str, unsigned short & port);
	auto inet_ntop(const sockaddr * addr) -> std::pair<std::string, unsigned short>;

	/// ::inet_pton wrapper, все строки в utf8
	/// @Return false если входная строка содержит не валидный адрес
	/// @Throws std::system_error в случае системной ошибки
	bool inet_pton(int family, const char * addr, sockaddr * out);
	bool inet_pton(int family, const std::string & addr, sockaddr * out);

	/// \{
	/// 
	/// ::getaddrinfo wrapper, все строки в utf8
	/// hints.ai_family = AF_UNSPEC
	/// hints.ai_protocol = IPPROTO_TCP
	/// hints.ai_socktype = SOCK_STREAM
	/// 
	/// @Param host имя или адрес как в ::getaddrinfo
	/// @Param service/port имя сервиса или номер порта как в ::getaddrinfo
	/// @Param err для nothrow overload, out параметр, тут будет ошибка, а возвращаемое значение будет null
	/// @Returns std::unique_ptr<addrinfo> resolved адрес, в случае ошибки - nullptr для error overloads
	/// @Throws std::system_error в случае системной ошибки

	addrinfo_ptr getaddrinfo(const char * host, const char * service);
	addrinfo_ptr getaddrinfo(const char * host, const char * service, std::error_code & err);

	inline addrinfo_ptr getaddrinfo(const std::string & host, const std::string & service, std::error_code & err) { return getaddrinfo(host.c_str(), service.c_str(), err); }
	inline addrinfo_ptr getaddrinfo(const std::string & host, std::error_code & err)                              { return getaddrinfo(host.c_str(), nullptr, err); }
	inline addrinfo_ptr getaddrinfo(const std::string & host, const std::string & service)                        { return getaddrinfo(host.c_str(), service.c_str()); }
	inline addrinfo_ptr getaddrinfo(const std::string & host)                                                     { return getaddrinfo(host.c_str(), nullptr); }

    /// \}

	/// Реализация streambuf для сокета на berkley socket api.
	/// Класс не thread-safe, кроме метода interrupt(не должен перемещаться/разрушаться во время вызова interrupt).
	/// умеет:
	/// * ввод/вывод
	/// * переключение в режим ssl с заданными ssl параметрами
	/// * прекращение ssl сессии и продолжение дальнейшей работы без ssl
	/// * timeout и interrupt для send, receive, connect операций.
	///   resolve, увы, не может быть реализован с поддержкой timeout и interrupt
	///   
	/// дополнительно от socket_streambuf_base:
	/// * установка пользовательского буфера, буфер делиться пополам на ввод/вывод(подробное смотри соотв метод)
	///   класс всегда буферезирован, минимальный размер буфера - смотри socket_streambuf_base,
	///   если пользователь попытается убрать буфер или предоставить слишком маленький буфер - класс откатится на буфер по умолчанию
	/// * входящая область по умолчанию автоматически синхронизируется с исходящей. Подобно std::ios::tie,
	///   как только входящий буфер исчерпан, прежде чем он будет заполнен из сокета - исходящий буфер будет сброшен в сокет
	///   
	/// IMPL NOTE:
	///  Реализация считает что для posix как минимум функции shutdown может быть вызвана из другого потока,
	///  и если было валидное соединение будет выполнена операция write, прерывающая блокирующее ожидание.
	///  Для прерывания connect'а используется временный вспомогательный pipe.
	///
	///  Реализация имеет внутреннее состояние по которому она узнает как делать interrupt для сокета в текущем состоянии.
	///  для состояния используется atomic_int
	class bsdsock_streambuf :
		public ext::socket_streambuf_base
	{
		typedef ext::socket_streambuf_base  base_type;
		typedef bsdsock_streambuf           self_type;

	public:
		typedef std::error_code       error_code_type;
		typedef std::system_error     system_error_type;

		typedef std::chrono::steady_clock::duration    duration_type;
		typedef std::chrono::steady_clock::time_point  time_point;

		typedef socket_handle_type    handle_type;

		enum // flags for wait_state
		{
			freadable = 1,  // wait readable
			fwritable = 2,  // wait writable
		};

	private:
		/// внутреннее состояние класса, нужно для поддержки вызова interrupt.
		/// все состояния кроме Interrupting / Interrupted последовательно меняется из основного потока работы.
		/// в состояние Interrupting / Interrupted класс может быть переведен в любой момент
		/// посредством вызова interrupt из любого другого потока, или signal handler'а
		enum StateType {
			Closed,        /// default state, закрытое состояние.
			Connecting,    /// выполняется попытка подключения вызовами non blocking connect + select.
			Opened,        /// подключение выполнено, нормальное рабочее состояние.
			Shutdowned,    /// для сокета был вызов shutdown, но еще не было вызова close.

			/// сокет в состояние прерывания, из другого потока/сущности происходит вызов shutdown/closesocket.
			/// В данное состояние можно перейти из любого другого состояния, в том числе и из Closed.
			Interrupting,

			/// работа сокета прервана вызовом interrupt.
			/// Единственный способ выйти из этого состояния - вызвать close, после чего сокет перейдет в состояние Closed
			Interrupted,
		};

	private:
		/// handle сокета, во время вызова connect здесь может лежать pipe handle для interrupt вызова.
		/// по окончанию вызова connect - тут handle socket'а.
		handle_type m_sockhandle = -1;

#if EXT_ENABLE_OPENSSL
		SSL * m_sslhandle = nullptr;
#endif

		std::atomic<StateType> m_state = {Closed};

		error_code_type m_lasterror;
		duration_type m_timeout = std::chrono::seconds(10); 

	private:
		/// публикует сокет для которого началось подключение,
		/// после публикации сокет доступен через m_sockhandle, а состояние изменяется в Connecting.
		/// Таким образом он может быть прерван вызовом closesocket из interrupt.
		/// returns true, если публикация была успешно, false если был запрос на прерывание
		bool publish_connecting(int pipe_handle);
		/// переводит состояние в Opened.
		/// returns true в случае успеха, false если был запрос на прерывание
		bool publish_opened(handle_type sock, StateType & expected);
		
		/// выполняет resolve с помощью getaddrinfo
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_resolve(const char * host, const char * service, addrinfo_type ** result);
		/// устанавливает не блокирующий режим работы сокета.
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_setnonblocking(handle_type sock);
		/// создает сокет с параметрами из addr.
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_createsocket(handle_type & sock, const addrinfo_type * addr);
		/// выполняет ::shutdown. в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_sockshutdown(handle_type sock, int how);
		/// выполняет ::closesocket. в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_sockclose(handle_type sock);

		/// выполняет подключение сокета sock. В процессе меняет состояние класса, публикует pipe handle для доступа из interrupt
		/// после выполнения m_sockhandle == sock. после успешного выполнения m_state == Opened.
		/// возвращает успех операции, в случае ошибки m_lasterror содержит код ошибки
		bool do_sockconnect(handle_type sock, const addrinfo_type * addr);
		bool do_sockconnect(handle_type sock, addrinfo_type * addr, unsigned short port);
		
		/// выполняет shutdown m_sockhandle, если еще не было interrupt
		bool do_shutdown();
		/// выполняет закрытие сокета, освобождение ssl объекта, переводит класс в закрытое состояние
		/// в случае ошибки возвращает false, а m_lasterror содержит ошибку
		bool do_close();
		/// выполняет попытку подключение класса: создание, настройка сокета -> подключение
		/// в случае ошибки возвращает false, а m_lasterror содержит ошибку
		bool do_connect(const addrinfo_type * addr);

		/// анализирует ошибку read/wrtie операции.
		/// res - результат операции recv/write, если 0 - то это eof и проверяется только State >=
		/// err - код ошибки операции errno/getsockopt(..., SO_ERROR, ...)
		/// В err_code записывает итоговую ошибку.
		/// возвращает была ли действительно ошибка, или нужно повторить операцию(реакция на EINTR).
		bool rw_error(int res, int err, error_code_type & err_code);
		
#ifdef EXT_ENABLE_OPENSSL
		error_code_type ssl_error(SSL * ssl, int error);
		/// анализирует ошибку ssl read/write операции.
		/// res[in] - результат операции(возращаяемое значение ::SSL_read, ::SSL_write).
		/// res[out] - результат ::SSL_get_error(ctx, res);
		/// В err_code записывает итоговую ошибку.
		/// возвращает была ли действительно ошибка, или нужно повторить операцию(реакция на EINTR).
		bool ssl_rw_error(int & res, error_code_type & err_code);
		/// создает ssl объект, ассоциирует его с дескриптором сокета и настраивает его.
		/// в случае ошибок возвращает false, ssl == nullptr, m_lasterror содержит ошибку
		bool do_createssl(SSL *& ssl, SSL_CTX * sslctx);
		/// выполняет ssl подключение(handshake)
		/// в случае ошибок возвращает false, m_lasterror содержит ошибку
		bool do_sslconnect(SSL * ssl);
		/// выполняет закрытие ssl сессии, не закрывает обычную сессию сокета(::shutdown не выполняется)
		/// в случае ошибок возвращает false, m_lasterror содержит ошибку
		bool do_sslshutdown(SSL * ssl);
#endif //EXT_ENABLE_OPENSSL

	public:
		/// ожидает пока сокет не станет доступен на чтение/запись(задается fstate) с помощью select.
		/// until - предельная точка ожидания.
		/// fstate должно быть комбинацией freadable, fwritable.
		/// в случае ошибки - возвращает false.
		/// учитывает WSAEINTR - повторяет ожидание, если только не было вызова interrupt
		bool wait_state(time_point until, int fstate);

		/// ожидает пока сокет не станет доступен на чтение с помощью select.
		/// until - предельная точка ожидания.
		/// в случае ошибки - возвращает false.
		/// учитывает WSAEINTR - повторяет ожидание, если только не было вызова interrupt
		bool wait_readable(time_point until) { return wait_state(until, freadable); }
		/// ожидает пока сокет не станет доступен на запись с помощью select.
		/// until - предельная точка ожидания.
		/// в случае ошибки - возвращает false.
		/// учитывает WSAEINTR - повторяет ожидание, если только не было вызова interrupt
		bool wait_writable(time_point until) { return wait_state(until, fwritable); }

	public:
		std::streamsize showmanyc() override;
		std::size_t read_some(char_type * data, std::size_t count) override;
		std::size_t write_some(const char_type * data, std::size_t count) override;

	public:
		/// timeout любых операций на сокетом,
		/// в случае превышения вызовы underflow/overflow/sync и другие вернут eof/-1/ошибку,
		/// а last_error() == ETIMEDOUT
		duration_type timeout() const { return m_timeout; }
		duration_type timeout(duration_type newtimeout);
		/// возвращает последнюю ошибку возникшую в ходе выполнения операции
		/// или ok если ошибок не было
		const error_code_type & last_error() const { return m_lasterror; }
		      error_code_type & last_error()       { return m_lasterror; }

		/// позволяет получить доступ к нижележащему сокету
		/// при изменении свойств сокета - никаких гарантий работы класса,
		/// можно использовать для получения свойств сокета, например local_endpoint/remove_endpoint.
		/// 
		/// socket != -1 гарантируется только при is_open() == true
		handle_type socket() { return m_sockhandle; }

		/// вызов ::getpeername(socket(), addr, addrlen), + проверка результат
		/// в случае ошибок кидает исключение system_error_type
		void getpeername(sockaddr_type * addr, socklen_t * addrlen);
		/// вызов ::getsockname(socket(), addr, namelen), + проверка результат
		/// в случае ошибок кидает исключение system_error_type
		void getsockname(sockaddr_type * addr, socklen_t * addrlen);

		/// возвращает строку адреса подключения вида <addr:port>(функция getpeername)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		std::string peer_endpoint();
		/// возвращает строку адреса и порт подключения (функция getpeername).
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		void peer_name(std::string & name, unsigned short & port);
		auto peer_name() -> std::pair<std::string, unsigned short>;
		/// возвращает строку адреса подключения (функция getpeername)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		std::string peer_address();
		/// возвращает порт подключения (функция getpeername)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		unsigned short peer_port();

		/// возвращает строку адреса подключения вида <addr:port>(функция getsockname)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		std::string sock_endpoint();
		/// возвращает строку адреса и порт подключения (функция getsockname).
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		void sock_name(std::string & name, unsigned short & port);
		auto sock_name() -> std::pair<std::string, unsigned short>;
		/// возвращает строку адреса подключения (функция getsockname)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		std::string sock_address();
		/// возвращает порт подключения (функция getsockname)
		/// в случае ошибок кидает исключение std::runtime_error / std::system_error
		unsigned short sock_port();

		/************************************************************************/
		/*                    управление подключением                           */
		/************************************************************************/
		/// подключение не валидно, если оно не открыто или была ошибка в процессе работы
		bool is_valid() const;
		/// подключение открыто, если была успешная попытка подключения,
		/// т.е. is_connected, по факту.
		bool is_open() const;

		/// выполняет подключение по заданным параметрам - в случае успеха возвращает true
		/// если подключение уже было выполнено - немедленно возвращает false
		bool connect(const addrinfo_type & addr);
		bool connect(const std::string & host, const std::string & service);
		bool connect(const std::string & host, unsigned short port);

#ifdef EXT_ENABLE_OPENSSL
		/// управление ssl сессией
		/// есть ли активная ssl сессия
		bool ssl_started() const;
		
		/// возвращает текущую SSL сессию. 
		/// если вызова start_ssl еще не было - returns nullptr, 
		/// тем не менее stop_ssl останавливает ssl соединение, но не удаляет сессию,
		/// повторный вызов start_ssl переиспользует ее.
		/// вызов close - освобождает ssl сессию.
		/// На данный момент нельзя передать объект сессии из вне.
		SSL * ssl_handle() { return m_sslhandle; }

		/// переключается в режим ssl c параметрами заданными последним вызовом bool start_ssl(SSL_CTX * sslctx)
		/// если такого вызова не было - аналогично start_ssl(SSLv23_client_method()).
		/// в случае ошибок возвращает false.
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl();

		/// выполняет установку ssl сессии с заданными параметрами - в случае успеха возвращает true
		/// данный метод подразумевает владение sslctx и взывает для него SSL_CTX_free
		/// в случае ошибок возвращает false.
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl(SSL_CTX * sslctx);
		
		/// выполняет установку ssl сессии с заданными параметрами - в случае успеха возвращает true
		/// данный метод не подразумевает владеет владение sslctx и НЕ взывает для него SSL_CTX_free
		/// в случае ошибок возвращает false.
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl_weak(SSL_CTX * sslctx);

		/// создает сессию с заданным методом и устанавливает ssl сессию
		/// в случае ошибок возвращает false.
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl(const SSL_METHOD * sslmethod);

		/// сбрасывает буфер и останавливает ssl сессию, если сессии не было - возвращает true
		/// если на каком либо из этапов возникла ошибка - returns false
		bool stop_ssl();

		/// Вызывает SSL_free(ssl_handle()). устанавливает его в nullptr.
		/// Следует вызвать только при закрытой сессии(stop_ssl).
		/// Автоматически вызывается в close. В целом обычно вызвать данный метод не следует.
		void free_ssl();
#endif

		/// если закрывается исходящее соединение - сбрасывает исходящий буфер
		/// выполняет shutdown для сокета, работает всегда с основным сокетом и не учитывает ssl сессию
		/// если возникает ошибка - returns false
		bool shutdown();

		/// сбрасывает исходящий буфер, останавливает ssl сессию,
		/// shutdowns socket. В любом случае закрывает сокет.
		/// переводит объект в рабочее default состояние.
		/// возвращает были ли ошибка при закрытии сокета.
		bool close();

		/// прерывает исполнение операции путем закрытия сокета,
		/// дальнейшее использование socket_streambuf запрещено, кроме как для закрытия/уничтожения объекта,
		/// после закрытия - можно повторно использовать
		/// может быть вызвано из любого потока, thread-safe
		/// предназначено для асинхронного принудительного закрытия, как обработчик сигналов(как пример Ctrl+C)/GUI программах/другое
		void interrupt();

	public:
		bsdsock_streambuf() BOOST_NOEXCEPT;
		~bsdsock_streambuf() BOOST_NOEXCEPT;

		bsdsock_streambuf(const bsdsock_streambuf &) = delete;
		bsdsock_streambuf & operator =(const bsdsock_streambuf &) = delete;

		bsdsock_streambuf(bsdsock_streambuf &&) BOOST_NOEXCEPT;
		bsdsock_streambuf & operator =(bsdsock_streambuf &&) BOOST_NOEXCEPT;

		void swap(bsdsock_streambuf & other) BOOST_NOEXCEPT;
	};

	inline void swap(bsdsock_streambuf & s1, bsdsock_streambuf & s2) BOOST_NOEXCEPT
	{
		s1.swap(s2);
	}

} // namespace ext
