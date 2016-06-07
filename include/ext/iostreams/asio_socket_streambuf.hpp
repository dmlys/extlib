#pragma once
#include <chrono>
#include <boost/config.hpp>
#include <boost/system/error_code.hpp>
#include <ext/iostreams/socket_streambuf_base.hpp>

/// на данный момент я не знаю простого способа форварднуть boost::asio
/// хочется иметь доступ к сокету, хотя бы для того что бы была возможность получить из него local_endpoint/remote_endpoint
/// но boost::asio::ip::tcp::socket - хитрый typedef в классе, класса который шаблонен,
/// пока не придумаю способ это все скрыть - делаем инклуды видимыми
#include <ext/iostreams/asio_config.hpp>
#include <boost/asio/ip/tcp.hpp>

/// define EXT_ENABLE_OPENSSL to enable openssl support
#ifdef EXT_ENABLE_OPENSSL
#include <boost/asio/ssl.hpp>
#endif

namespace ext
{
	/// реализация streambuf для сокета на основе boost::asio.
	/// для реализации используется boost::asio
	/// Данная библиотека обеспечивает необходимую портабельность,
	/// позволяет реализовать все необходимые операции с timeout'ами,
	/// достаточно легко добавить поддержку ssl на основе openssl.
	/// 
	/// без нее пришлось бы писать как минимум 2 реализации, для posix и windows, причем они отличались бы флагами,
	/// некоторыми функциями(например установка флагов неблокирующего режима, timeout'ов).
	/// так же добавление openssl возможно было бы сложнее.
	/// TODO: Неплохо бы сделать реализацию на berkley socket API с поддержкой openssl и сравнить
	/// + на boost::asio достаточно легко добавить asynchronus interrupt
	/// 
	/// ----!!! IMPORTANT !!!----
	/// при создании класса, ему можно передать shared_ptr<io_service> объект.
	/// Все asio_socket_streambuf объекты разделяющие общий io_service должны использоваться из одного и того же потока!
	///
	/// Это обусловлено тем что внутри класса вызывается run_one в цикле, пока не отработают обработчики операций,
	/// если run_one будет вызваться параллельно в другом потоке - обработчики данного объекта могут отработать в другом потоке,
	/// а данный поток может не выйти из из своего run_one, например потому что больше обработчиков нет + такая же ситуация возможна в другом потоке.
	/// причем boost::asio::io_service::strand тут не поможет.
	/// 
	/// исключение метод interrupt - может вызваться из любого потока
	/// на каждый boost::io_service свой поток
	/// -------------------------
	/// 
	/// умеет:
	/// * ввод/вывод
	/// * переключение в режим ssl с заданными ssl параметрами
	/// * прекращение ssl сессии и продолжение дальнейшей работы без ssl
	/// * timeout - на каждую операцию, т.е. любая совершаемая операция должна уложится в заданный промежуток времени
	/// 
	/// дополнительно от socket_streambuf_base:
	/// * установка пользовательского буфера, буфер делиться пополам на ввод/вывод(подробное смотри соотв метод)
	///   класс всегда буферезирован, минимальный размер буфера - смотри socket_streambuf_base
	///   если пользователь попытается убрать буфер или предоставить слишком маленький буфер - класс откатится на буфер по умолчанию
	/// * входящая область по умолчанию автоматически синхронизируется с исходящей. Подобно std::ios::tie
	///   как только входящий буфер исчерпан, прежде чем он будет заполнен из сокета - исходящий буфер будет сброшен в сокет
	class asio_socket_streambuf :
		public socket_streambuf_base
	{
	private:
		typedef socket_streambuf_base base_type;

		class device;
		std::unique_ptr<device> m_device;

	public:
		typedef boost::system::error_code error_code_type;
		typedef boost::system::system_error system_error_type;
		typedef std::chrono::system_clock::duration duration_type;

	protected:
		std::streamsize showmanyc() override;
		std::size_t read_some(char_type * data, std::size_t count) override;
		std::size_t write_some(const char_type * data, std::size_t count) override;

	public:
		/// timeout любых операций на сокетом,
		/// в случае превышения вызовы underflow/overflow/sync и другие вернут eof/-1/ошибку,
		/// а last_error() == boost::asio::error::timeout
		duration_type timeout() const;
		duration_type timeout(duration_type newtimeout);
		/// возвращает последнюю ошибку возникшую в ходе выполнения операции
		/// или ok если ошибок не было
		const error_code_type & last_error() const;
		      error_code_type & last_error();

		/// сервис с которым был создан сокет
		std::shared_ptr<boost::asio::io_service> service() const;
		/// позволяет получить доступ к нижележащему сокету
		/// при изменении свойств сокета - никаких гарантий работы класса,
		/// можно использовать для получения свойств сокета, например local_endpoint/remove_endpoint
		boost::asio::ip::tcp::socket & socket();

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

		/// подключение открыто, если была попытка подключения, даже если не успешная.
		/// если resolve завершился неудачей - класс не считается открытым
		bool is_open() const;
		/// выполняет подключение по заданным параметрам - в случае успеха возвращает true
		/// если подключение уже было выполнено - немедленно возвращает false
		bool connect(const std::string & host, unsigned short port);
		bool connect(const std::string & host, const std::string & service);
		bool connect(const boost::asio::ip::tcp::endpoint & ep);

#ifdef EXT_ENABLE_OPENSSL
		/// управление ssl сессией
		bool ssl_started() const;
		/// переключается в режим ssl
		/// boost::asio::ssl::context_base::sslv23_client,
		/// boost::asio::ssl::context_base::verify_none
		bool start_ssl();
		/// выполняет установку ssl сессии с заданными параметрами - в случае успеха возвращает true
		/// если сессию уже была установлена - немедленно возвращает false
		bool start_ssl(boost::asio::ssl::context ctx);

		/// останавливает ssl сессию,
		/// если на каком либо из этапов возникла ошибка - returns false, при этом сокет закрывается
		/// если ssl сессия не была открыта - returns false, сокет не закрывает
		/// 
		/// сбрасывает выходной буфер, и если не произошло ошибок - пытается закрыть ssl сессию:
		///   если сервер корректно присылает close_notify -
		///     сокет остается открытым и возвращается в обычный, не защищенный режим работы
		///     return true
		///   если же сервер не присылает close_notify, а сразу закрывает соединение -
		///     класс также закрывает соединение/сокет. is_connected() == false;
		///     return true
		///   если сервер нарушает протокол/не отвечает - ошибка/timeout, returns false
		///   может быть интересным:
		///   http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		///   http://stackoverflow.com/a/25703699/1682317
		bool stop_ssl();
#endif
		/// если закрывается исходящее соединение - сбрасывает исходящий буфер
		/// выполняет shutdown для сокета, работает всегда с основным сокетом и не учитывает ssl сессию
		/// если возникает ошибка - returns false и закрывает сокет
		bool shutdown();

		/// сбрасывает исходящий буфер, останавливает ssl сессию,
		/// shutdowns socket. В любом случае закрывает сокет
		/// discards interrupt requests if any
		/// возвращает была ли ошибка
		bool close();

		/// прерывает исполнение операции путем закрытия сокета,
		/// дальнейшее использование asio_socket_streambuf запрещено, кроме как для закрытия/уничтожения объекта,
		/// после закрытия - можно повторно использовать
		/// может быть вызвано из любого потока, thread-safe
		/// предназначено для асинхронного принудительного закрытия, как обработчик сигналов(как пример Ctrl+C)/GUI программах/другое
		void interrupt();

	public:
		asio_socket_streambuf();
		asio_socket_streambuf(std::shared_ptr<boost::asio::io_service> ioserv);
		~asio_socket_streambuf() BOOST_NOEXCEPT;

		asio_socket_streambuf(const asio_socket_streambuf &) = delete;
		asio_socket_streambuf & operator =(const asio_socket_streambuf &) = delete;

		asio_socket_streambuf(asio_socket_streambuf &&) BOOST_NOEXCEPT;
		asio_socket_streambuf& operator =(asio_socket_streambuf &&) BOOST_NOEXCEPT;

		void swap(asio_socket_streambuf & right) BOOST_NOEXCEPT;
	};

	inline void swap(asio_socket_streambuf & s1, asio_socket_streambuf & s2) BOOST_NOEXCEPT
	{
		s1.swap(s2);
	}
}
