#pragma once
// author: Dmitry Lysachenko
// date: Saturday 30 august 2015
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <cstdint>
#include <memory>
#include <atomic>
#include <utility>
#include <string>
#include <chrono>
#include <system_error>

#include <ext/iostreams/socket_streambuf_base.hpp>

//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

// #include <WinSock2.h>
// #include <WS2tcpip.h>
// #include <Windows.h>

// forward from WinSock2.h, used in private methods signatures
struct addrinfoW;
struct sockaddr;

#ifdef EXT_ENABLE_OPENSSL
/// forward some openssl types
struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;

typedef struct ssl_st        SSL;
typedef struct ssl_ctx_st    SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
#endif //EXT_ENABLE_OPENSSL

namespace ext
{
	typedef	addrinfoW addrinfo_type;
	typedef sockaddr sockaddr_type;
	typedef std::uintptr_t socket_handle_type;

	struct addrinfo_deleter
	{
		void operator()(addrinfo_type * ptr) const;
	};

	typedef std::unique_ptr<addrinfo_type, addrinfo_deleter> addrinfo_ptr;

	/// функции инициализации winsock библиотеки, вынесены сюда для изоляции include'ов windows
	/// вызывает WSAstartup и возвращает результат выполнения
	int wsastartup(std::uint16_t version);
	/// вызывает WSAstartup с версией 2.2.
	/// если вызов завершился с неудачей, выводит сообщение в std::cerr и вызывает std::exit(EXIT_FAILURE).
	/// для лучшего контроля - вызывайте WSAStartup напрямую или int wsastartup(std::uint16_t version)
	void wsastartup();
	/// вызывает WSACleanup
	void wsacleanup();

	/// производит инициализацию библиотек необходимых для использования winsock2_streambuf.
	/// это winosock2 и, если включен, OpenSSL
	/// по факту вызывает ext::wsastratup(); ext::openssl_init()
	void winsock2_stream_init();

	BOOST_NORETURN void throw_winsock2_error(int code, const char * errmsg);
	BOOST_NORETURN void throw_winsock2_error(int code, const std::string & errmsg);

	/// ::inet_ntop wrapper для winsock платформы, все строки в utf8(utf16 для wchar_t)
	/// @Throws std::system_error в случае системной ошибки
	///         ext::codecvt_convert::conversion_failure(std::runtime_error derived)
	///              при ошибках конвертации utf-8 <-> utf-16 кидает
	void inet_ntop(const sockaddr * addr, std::wstring & wstr, unsigned short & port);
	void inet_ntop(const sockaddr * addr, std::string & str, unsigned short & port);
	auto inet_ntop(const sockaddr * addr)->std::pair<std::string, unsigned short>;

	/// ::inet_pton wrapper для winsock платформы, все строки в utf8(utf16 для wchar_t)
	/// @Return false если входная строка содержит не валидный адрес
	/// @Throws std::system_error в случае системной ошибки
	///         ext::codecvt_convert::conversion_failure(std::runtime_error derived)
	///              при ошибках конвертации utf-8 <-> utf-16 кидает
	bool inet_pton(int family, const wchar_t * waddr, sockaddr * out);
	bool inet_pton(int family, const char * addr, sockaddr * out);
	bool inet_pton(int family, const std::wstring & waddr, sockaddr * out);
	bool inet_pton(int family, const std::string & addr, sockaddr * out);

	/// \{
	/// 
	/// ::getaddrinfo wrapper для winsock платформы, все строки в utf8(utf16 для wchar_t)
	/// hints.ai_family = AF_UNSPEC
	/// hints.ai_protocol = IPPROTO_TCP
	/// hints.ai_socktype = SOCK_STREAM
	/// 
	/// @Param host имя или адрес как в ::getaddrinfo
	/// @Param service/port имя сервиса или номер порта как в ::getaddrinfo
	/// @Param err для nothrow overload, out параметр, тут будет ошибка, а возвращаемое значение будет null
	/// @Returns std::unique_ptr<addrinfo> resolved адрес, в случае ошибки - nullptr для error overloads
	/// @Throws std::system_error в случае системной ошибки
	///         ext::codecvt_convert::conversion_failure(std::runtime_error derived)
	///              при ошибках конвертации utf-8 <-> utf-16 кидает

	addrinfo_ptr getaddrinfo(const wchar_t * host, const wchar_t * service);
	addrinfo_ptr getaddrinfo(const wchar_t * host, const wchar_t * service, std::error_code & err);

	addrinfo_ptr getaddrinfo(const char * host, const char * service);
	addrinfo_ptr getaddrinfo(const char * host, const char * service, std::error_code & err);
	

	inline addrinfo_ptr getaddrinfo(const std::wstring & host, const std::wstring & service, std::error_code & err)  { return getaddrinfo(host.c_str(), service.c_str(), err); }
	inline addrinfo_ptr getaddrinfo(const std::wstring & host, std::error_code & err)                                { return getaddrinfo(host.c_str(), nullptr, err); }
	inline addrinfo_ptr getaddrinfo(const std::wstring & host, const std::wstring & service)                         { return getaddrinfo(host.c_str(), service.c_str()); }
	inline addrinfo_ptr getaddrinfo(const std::wstring & host)                                                       { return getaddrinfo(host.c_str(), nullptr); }

	inline addrinfo_ptr getaddrinfo(const std::string & host, const std::string & service, std::error_code & err) { return getaddrinfo(host.c_str(), service.c_str(), err); }
	inline addrinfo_ptr getaddrinfo(const std::string & host, std::error_code & err)                              { return getaddrinfo(host.c_str(), nullptr, err); }
	inline addrinfo_ptr getaddrinfo(const std::string & host, const std::string & service)                        { return getaddrinfo(host.c_str(), service.c_str()); }
	inline addrinfo_ptr getaddrinfo(const std::string & host)                                                     { return getaddrinfo(host.c_str(), nullptr); }

    /// \}


	/// Реализация streambuf для сокета на winsock2 функциях(winsock < 2.0 не поддерживается).
	/// Класс не thread-safe, кроме метода interrupt(не должен перемещаться/разрушаться во время вызова interrupt).
	/// умеет:
	/// * ввод/вывод
	/// * переключение в режим ssl с заданными ssl параметрами
	/// * прекращение ssl сессии и продолжение дальнейшей работы без ssl
	/// * timeout и interrupt для send, receive, connect операций.
	///   resolve увы не может быть реализован с поддержкой timeout и interrupt
	///   
	/// дополнительно от socket_streambuf_base:
	/// * установка пользовательского буфера, буфер делиться пополам на ввод/вывод(подробное смотри соотв метод)
	///   класс всегда буферезирован, минимальный размер буфера - смотри socket_streambuf_base
	///   если пользователь попытается убрать буфер или предоставить слишком маленький буфер - класс откатится на буфер по умолчанию
	/// * входящая область по умолчанию автоматически синхронизируется с исходящей. Подобно std::ios::tie,
	///   как только входящий буфер исчерпан, прежде чем он будет заполнен из сокета - исходящий буфер будет сброшен в сокет
	///   
	/// IMPL NOTE:
	///  поскольку данный класс реализация строго под winsock2(и возможно выше)
	///  мы можем закладываться на особенности WINAPI платформы.
	///  для WINAPI класс сокета потоко-безопасен, а вызов close/shutdown прерывает блокирующий select.
	///  реализация имеет внутреннее состояние по которому она узнает как делать interrupt для сокета в текущем состоянии.
	///  для состояния используется atomic_int
	class winsock2_streambuf :
		public ext::socket_streambuf_base
	{
		typedef ext::socket_streambuf_base base_type;
		typedef winsock2_streambuf self_type;

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

		static const std::string empty;
		static const std::wstring wempty;

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
		handle_type m_sockhandle; // = INVALID_SOCKET;

#ifdef EXT_ENABLE_OPENSSL
		SSL * m_sslhandle = nullptr;
#endif // EXT_ENABLE_OPENSSL

		std::atomic<StateType> m_state { Closed };

		error_code_type m_lasterror;
		duration_type m_timeout = std::chrono::seconds(10);

	private:
		/// публикует сокет для которого началось подключение,
		/// после публикации сокет доступен через m_sockhandle, а состояние изменяется в Connecting.
		/// Таким образом он может быть прерван вызовом closesocket из interrupt.
		/// returns true, если публикация была успешно, false если был запрос на прерывание
		bool publish_connecting(handle_type sock);
		/// переводит состояние в Opened.
		/// returns true в случае успеха, false если был запрос на прерывание
		bool publish_opened(handle_type sock, StateType & expected);
		
		/// выполняет resolve с помощью getaddrinfo
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_resolve(const wchar_t * host, const wchar_t * service, addrinfo_type ** result);
		/// устанавливает не блокирующий режим работы сокета.
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_setnonblocking(handle_type sock);
		/// создает сокет с параметрами из addr.
		/// в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_createsocket(handle_type & sock, const addrinfo_type * addr);
		/// выполняет ::shutdown. в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_sockshutdown(handle_type sock);
		/// выполняет ::closesocket. в случае ошибки - устанавливает m_lasterror и возвращает false
		bool do_sockclose(handle_type sock);

		/// выполняет подключение сокета sock. В процессе меняет состояние класса, публикует сокет для доступа из interrupt
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
		/// создает ssl объект, ассоциирует его с дескриптором сокета
		/// в случае ошибок возвращает false, ssl == nullptr, m_lasterror содержит ошибку
		bool do_createssl(SSL *& ssl, SSL_CTX * sslctx);
		/// ассоциирует sslctx c ssl, выставляет servername,
		/// в случае ошибок возвращает false, ssl == nullptr, m_lasterror содержит ошибку
		bool do_configuressl(SSL *& ssl, const char * servername = nullptr);
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
		/// timeout любых операций над сокетом,
		/// в случае превышения вызовы underflow/overflow/sync и другие вернут eof/-1/ошибку,
		/// а last_error() == WSAETIMEDOUT
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
		/// socket != INVALID_SOCKET гарантируется только при is_open() == true
		handle_type socket() { return m_sockhandle; }

		/// вызов ::getpeername(socket(), addr, addrlen), + проверка результат
		/// в случае ошибок кидает исключение system_error_type
		void getpeername(sockaddr_type * addr, int * addrlen);
		/// вызов ::getsockname(socket(), addr, namelen), + проверка результат
		/// в случае ошибок кидает исключение system_error_type
		void getsockname(sockaddr_type * addr, int * addrlen);

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
		bool connect(const std::wstring & host, const std::wstring & service);
		bool connect(const std::wstring & host, unsigned short port);

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
		SSL * ssl_handle() { return m_sslhandle; }

		/// устанавливает SSL сессию.
		/// Если уже есть активная(ssl_started) сессия - кидает std::logic_error исключение.
		/// Последующий вызов start_ssl будет использовать данную сессию.
		/// Данный метод подразумевает владение ssl и взывает для него SSL_free в методе close
		void set_ssl(SSL * ssl);

		/// переключается в режим ssl c параметрами заданными последним вызовом bool start_ssl(SSL_CTX * sslctx)
		/// если такого вызова не было - аналогично start_ssl(SSLv23_client_method()).
		/// в случае ошибок возвращает false.
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl();

		/// создает сессию с заданным методом и servername, выполняет ssl соединение, в случае ошибок возвращает false.
		/// servername - ::SSL_set_tlsext_host_name(ssl, servername), он же ключ -servername у openssl.exe
		/// NOTE: метод не проверяет наличие активной сессии
		bool start_ssl(const SSL_METHOD * sslmethod)     { return start_ssl(sslmethod, wempty); }
		bool start_ssl(const std::string & servername)   { return start_ssl(nullptr, servername); }
		bool start_ssl(const std::wstring & wservername) { return start_ssl(nullptr, wservername); }
		bool start_ssl(const SSL_METHOD * sslmethod, const std::string & servername);
		bool start_ssl(const SSL_METHOD * sslmethod, const std::wstring & wservername);

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
		winsock2_streambuf() BOOST_NOEXCEPT;
		~winsock2_streambuf() BOOST_NOEXCEPT;

		winsock2_streambuf(const winsock2_streambuf &) = delete;
		winsock2_streambuf & operator =(const winsock2_streambuf &) = delete;

		winsock2_streambuf(winsock2_streambuf &&) BOOST_NOEXCEPT;
		winsock2_streambuf & operator =(winsock2_streambuf &&) BOOST_NOEXCEPT;

		void swap(winsock2_streambuf & other) BOOST_NOEXCEPT;
	};

	inline void swap(winsock2_streambuf & s1, winsock2_streambuf & s2) BOOST_NOEXCEPT
	{
		s1.swap(s2);
	}
}
