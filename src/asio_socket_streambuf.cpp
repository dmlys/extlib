#include <ext/iostreams/asio_socket_streambuf.hpp>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/asio/system_timer.hpp>

namespace ext
{
	/// реализация streambuf для сокета.
	/// для реализации используется boost::asio, схема следующая:
	/// когда требуется сделать что-то с сокетом,
	/// мы инициируем асинхронный запрос(чтение, резолв, ...) и запускаем таймер.
	/// если запрос выполнился раньше таймера - операция успешна, а таймер отменяется
	/// если таймер отрабатывает раньше запроса - операция отменяется.
	/// 
	/// в любом случае обработчики отработают, это гарантируется asio,
	/// а значит они должны понимать в каких условиях были вызваны, успешная операция, или отмена.
	/// так же возможна ситуация, когда обработчик уже попал в очередь, но не успел отработать,
	/// при этом в очередь добавляется парный ему обработчик таймера/запроса
	/// эти ситуации должны корректно обрабатываться.
	/// 
	/// для реализации условий выше, обработчики берут помимо прочих параметров, указатель на общий счетчик, изначально инициализированный в 0
	/// при выполнении, обработчик проверяет данный счетчик, и если он больше 0 - значит мы опоздали
	/// поток ожидания выполняет m_ioserv.run_one пока счетчик не станет равным 2 - значит оба обработчика отработали
	/// 
	/// interrupt:
	/// так же мы хотим уметь прерывать текущее действие из другого потока, ситуация усложняется тем,
	/// что помимо работы с сокетом, есть еще resolve, который тоже может быть длительным.
	/// 
	/// 1. Всегда можно вызвать asio::ioservice.stop(), но после этого сервис и сокеты нужно будет пересоздать,
	///    причем именно уничтожить и создать по новой объекты, это касается всех сокетов разделяющих один и тот же ioservice.
	///    однако на практике такая схема не работает под windows, увы
	///    stop как-то не прерывает run/run_one - как-то что-то внутри не так
	///    
	/// 2. поступаем следующим образом. добавляем действие в asio::ioservice.
	///    устанавливаем код ошибки в boost::asio::error::interrupted,
	///    отменяем timer, обработчик timer'а для resolve, проверяет изменился ли код на interrupted -> отменяет resolve
	///    в лубом случае закрываем сокет - если с ним выполнялась какая-либо операция - она сбросится,
	///    если мы попали между ожиданиями - сокет закроется при следующем обращении, а запрошенная операция тут же завершиться с ошибкой

	class asio_socket_streambuf::device
	{
		std::shared_ptr<boost::asio::io_service> m_ioserv;
		boost::asio::ip::tcp::socket m_sock;
		boost::asio::system_timer m_timer;

#ifdef EXT_ENABLE_OPENSSL
		typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket &> ssl_stream;
		std::unique_ptr<ssl_stream> m_sslstream;
		std::unique_ptr<boost::asio::ssl::context> m_sslcontext;
#endif

		std::chrono::system_clock::duration m_timeout;
		boost::system::error_code m_last_error;
		int m_callback_counter;              // счетчик нормальных колбаков
		std::atomic_int m_interrupt_counter; // счетчик interrupt запросов

	private:
		// вспомогательные функции обработки callback'ов

		/// инкрементируют счетчик
		/// если callback пришел первым - выставляет last_error в ec, отменяет timer и возвращает true
		///               иначе - возвращает false
		inline bool on_complete(const boost::system::error_code & ec)
		{
			bool first = m_callback_counter++ == 0 && m_last_error != boost::asio::error::interrupted;
			if (first)
			{
				m_last_error = ec;
				boost::system::error_code ignored;
				m_timer.cancel(ignored);
			}

			return first;
		}

		/// инкрементируют счетчик
		/// если callback пришел первым - выставляет last_error в timed_out и возвращает true
		///               иначе - возвращает false
		inline bool on_timer()
		{
			bool first = m_callback_counter++ == 0;
			if (first && m_last_error != boost::asio::error::interrupted)
				m_last_error = boost::asio::error::timed_out;

			return first;
		}

		inline bool pending()
		{
			return m_callback_counter != 2;
		}

	private: // asynchronous request handlers
		struct resolver_handler;
		struct resolver_canceler;

		struct transfer_error_code;
		struct transfer_handler;

		template <class Socket>
		struct socket_closer;

		template <class Socket>
		inline socket_closer<Socket> make_socket_closer(device * owner, Socket & sock) { return socket_closer<Socket>(owner, sock); }

	private:
		void resolve(const boost::asio::ip::tcp::resolver::query & q,
		             boost::asio::ip::tcp::endpoint & ep);
		
#ifdef EXT_ENABLE_OPENSSL
		void handshake();
		void ssl_shutdown();
#endif

		template <class Socket>
		std::size_t write(Socket & sock, const char * data, std::size_t count);

		template <class Socket>
		std::size_t read(Socket & sock, char * data, std::size_t count);

		void do_connect(const boost::asio::ip::tcp::endpoint & ep);

	public:
		device(std::shared_ptr<boost::asio::io_service> ioserv);
		~device() = default;
		void reset();

		const boost::system::error_code & last_error() const { return m_last_error; }
		      boost::system::error_code & last_error()       { return m_last_error; }

		std::shared_ptr<boost::asio::io_service> service() const { return m_ioserv; }
		boost::asio::ip::tcp::socket & socket() { return m_sock; }

		std::chrono::system_clock::duration timeout() const { return m_timeout; }
		std::chrono::system_clock::duration timeout(std::chrono::system_clock::duration newtimeout)
		{ return ext::exchange(m_timeout, newtimeout); }


		bool is_open() { return m_sock.is_open(); }
		void connect(const std::string & host, unsigned short port);
		void connect(const std::string & host, const std::string & service);
		void connect(const boost::asio::ip::tcp::endpoint & ep);
		void shutdown();
		void close();
		void interrupt();

#ifdef EXT_ENABLE_OPENSSL
		bool ssl_started() { return static_cast<bool>(m_sslstream); }
		void start_ssl(boost::asio::ssl::context ctx);
		void stop_ssl();
#endif

		std::size_t read_some(char * data, std::size_t count);
		std::size_t write_some(const char * data, std::size_t count);
	}; // struct asio_socket_streambuf::device

	/************************************************************************/
	/*          asio_socket_streambuf::device callbacks implementation           */
	/************************************************************************/
	struct asio_socket_streambuf::device::resolver_handler
	{
		device * owner;
		boost::asio::ip::tcp::resolver::iterator * m_it;


		resolver_handler(device * owner, boost::asio::ip::tcp::resolver::iterator & it)
			: owner(owner), m_it(&it) {}

		void operator()(const boost::system::error_code & ec, boost::asio::ip::tcp::resolver::iterator it) const
		{
			if (owner->on_complete(ec))
				*m_it = std::move(it);
		}
	};

	struct asio_socket_streambuf::device::resolver_canceler
	{
		device * owner;
		boost::asio::ip::tcp::resolver * m_resolver;
		
		resolver_canceler(device * owner, boost::asio::ip::tcp::resolver & resolver)
			: owner(owner), m_resolver(&resolver) {}

		void operator()(const boost::system::error_code & ec)
		{
			// ignore(ec). если мы первые - в независимости от ec отменяем resolve,
			if (owner->on_timer())
				m_resolver->cancel();
		}
	};

	template <class Socket>
	struct asio_socket_streambuf::device::socket_closer
	{
		device * owner;
		Socket * m_sock;

		socket_closer(device * owner, Socket & sock)
			: owner(owner), m_sock(&sock) {}

		void operator()(const boost::system::error_code & ec)
		{
			if (owner->on_timer())
			{
				boost::system::error_code ignored;
				m_sock->lowest_layer().close(ignored);
			}
		}
	};

	struct asio_socket_streambuf::device::transfer_error_code
	{
		device * owner;
		transfer_error_code(device * owner) : owner(owner) {}

		void operator()(const boost::system::error_code & ec) const
		{
			owner->on_complete(ec);
		}
	};

	struct asio_socket_streambuf::device::transfer_handler
	{
		device * owner;
		std::size_t * m_transfered;

		transfer_handler(device * owner, std::size_t & tranfered)
			: owner(owner), m_transfered(&tranfered) {}

		void operator()(const boost::system::error_code & ec, std::size_t transfered) const
		{
			if (owner->on_complete(ec))
				*m_transfered = transfered;
		}
	};

	/************************************************************************/
	/*          asio_socket_streambuf::device methods implementation             */
	/************************************************************************/
	void asio_socket_streambuf::device::do_connect(const boost::asio::ip::tcp::endpoint & ep)
	{
		m_callback_counter = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(make_socket_closer(this, m_sock));
		m_sock.async_connect(ep, transfer_error_code(this));

		do m_ioserv->run_one(); while (pending());
	}

	void asio_socket_streambuf::device::resolve(const boost::asio::ip::tcp::resolver::query & q, boost::asio::ip::tcp::endpoint & ep)
	{
		boost::asio::ip::tcp::resolver resolver {*m_ioserv};
		boost::asio::ip::tcp::resolver::iterator it;
		m_callback_counter = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(resolver_canceler(this, resolver));
		resolver.async_resolve(q, resolver_handler(this, it));

		do m_ioserv->run_one(); while (pending());
		if (!m_last_error) ep = *it;
	}

	inline void asio_socket_streambuf::device::connect(const boost::asio::ip::tcp::endpoint & ep)
	{
		m_last_error.clear();
		do_connect(ep);
	}

	void asio_socket_streambuf::device::connect(const std::string & host, unsigned short port)
	{
		m_last_error.clear();

		boost::asio::ip::tcp::endpoint ep;
		resolve({host, ""}, ep);
		if (m_last_error) return;

		ep = {ep.address(), port};
		connect(ep);
	}

	void asio_socket_streambuf::device::connect(const std::string & host, const std::string & service)
	{
		m_last_error.clear();

		boost::asio::ip::tcp::endpoint ep;
		resolve({host, service}, ep);
		if (m_last_error) return;

		connect(ep);
	}

#ifdef EXT_ENABLE_OPENSSL
	void asio_socket_streambuf::device::handshake()
	{
		m_callback_counter = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(make_socket_closer(this, m_sock));
		m_sslstream->async_handshake(boost::asio::ssl::stream_base::client, transfer_error_code(this));

		do m_ioserv->run_one(); while (pending());
	}

	void asio_socket_streambuf::device::ssl_shutdown()
	{
		m_callback_counter = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(make_socket_closer(this, m_sock));
		m_sslstream->async_shutdown(transfer_error_code(this));

		do m_ioserv->run_one(); while (pending());
	}
#endif

	template <class Socket>
	std::size_t asio_socket_streambuf::device::read(Socket & sock, char * data, std::size_t count)
	{
		m_callback_counter = 0;
		std::size_t readed = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(make_socket_closer(this, sock));
		sock.async_read_some(boost::asio::buffer(data, count), transfer_handler(this, readed));

		do m_ioserv->run_one(); while (pending());
		return readed;
	}

	template <class Socket>
	std::size_t asio_socket_streambuf::device::write(Socket & sock, const char * data, std::size_t count)
	{
		m_callback_counter = 0;
		std::size_t written = 0;

		m_timer.expires_from_now(m_timeout);
		m_timer.async_wait(make_socket_closer(this, sock));
		sock.async_write_some(boost::asio::buffer(data, count), transfer_handler(this, written));

		do m_ioserv->run_one(); while (pending());
		return written;
	}

	/************************************************************************/
	/*                publics                                               */
	/************************************************************************/
	asio_socket_streambuf::device::device(std::shared_ptr<boost::asio::io_service> ioserv) :
		m_ioserv(ioserv),
		m_sock(*m_ioserv), m_timer(*m_ioserv)
	{
		m_interrupt_counter.store(0, std::memory_order_relaxed);
		m_timeout = std::chrono::seconds(10);
	}

	void asio_socket_streambuf::device::reset()
	{
		m_interrupt_counter.store(0, std::memory_order_relaxed);
		m_last_error.clear();
	}
	
	void asio_socket_streambuf::device::close()
	{
		boost::system::error_code ignored;
		m_timer.cancel(ignored);
		m_sock.close(ignored);

#ifdef EXT_ENABLE_OPENSSL
		m_sslstream = nullptr;
		m_sslcontext = nullptr;
#endif

		// нужно обработать и дождаться всех interrupt request'ов
		while (m_interrupt_counter.load(std::memory_order_consume) > 0)
			m_ioserv->run_one();
	}

	void asio_socket_streambuf::device::interrupt()
	{
		auto interrupter = [this]
		{
			// {boost::system::errc::interrupted, boost::system::generic_category()};
			m_last_error = boost::asio::error::interrupted;
			boost::system::error_code ignored;
			m_timer.cancel(ignored);
			m_sock.close(ignored);
			m_interrupt_counter.fetch_sub(1, std::memory_order_relaxed);
		};

		m_interrupt_counter.fetch_add(1, std::memory_order_relaxed);
		m_ioserv->post(interrupter);
	}

	void asio_socket_streambuf::device::shutdown()
	{
		m_sock.shutdown(m_sock.shutdown_both, m_last_error);
	}

#ifdef EXT_ENABLE_OPENSSL
	void asio_socket_streambuf::device::start_ssl(boost::asio::ssl::context ctx)
	{
		m_sslcontext = std::make_unique<boost::asio::ssl::context>(std::move(ctx));
		m_sslstream = std::make_unique<ssl_stream>(m_sock, *m_sslcontext);

		handshake();
	}

	void asio_socket_streambuf::device::stop_ssl()
	{
		ssl_shutdown();

		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		// http://stackoverflow.com/a/25703699/1682317
		// мы послали close_notify, но нам не ответили,
		// boost::asio::ssl транслирует такие случаи в SSL_R_SHORT_READ
		// нам ничего не остается только как проигнорировать, и/или закрыть соединение
		auto & err = m_last_error;
		bool no_shutdown_notify =
			err.category() == boost::asio::error::get_ssl_category() &&
			ERR_GET_REASON(err.value()) == SSL_R_SHORT_READ;

		if (no_shutdown_notify)
		{
			err.clear();
			close();
		}

		m_sslstream.reset();
		m_sslcontext.reset();
	}
#endif

	std::size_t asio_socket_streambuf::device::read_some(char * data, std::size_t count)
	{
#ifdef EXT_ENABLE_OPENSSL
		if (m_sslstream)
			return read(*m_sslstream, data, count);
		else
			return read(m_sock, data, count);
#else
		return read(m_sock, data, count);
#endif
	}

	std::size_t asio_socket_streambuf::device::write_some(const char * data, std::size_t count)
	{
#ifdef EXT_ENABLE_OPENSSL
		if (m_sslstream)
			return write(*m_sslstream, data, count);
		else
			return write(m_sock, data, count);
#else
		return write(m_sock, data, count);
#endif
	}

	/************************************************************************/
	/*               asio_socket_streambuf                                  */
	/************************************************************************/
	/************************************************************************/
	/*               state changing functions                               */
	/************************************************************************/
	bool asio_socket_streambuf::is_open() const
	{
		return m_device->is_open();
	}

	bool asio_socket_streambuf::connect(const std::string & host, unsigned short port)
	{
		if (is_open())
			return false;

		init_buffers();
		m_device->connect(host, port);
		return !m_device->last_error();
	}

	bool asio_socket_streambuf::connect(const std::string & host, const std::string & service)
	{
		if (is_open())
			return false;

		init_buffers();
		m_device->connect(host, service);
		return !m_device->last_error();
	}

	bool asio_socket_streambuf::connect(const boost::asio::ip::tcp::endpoint & ep)
	{
		if (is_open())
			return false;

		init_buffers();
		m_device->connect(ep);
		return !m_device->last_error();
	}

#ifdef EXT_ENABLE_OPENSSL
	bool asio_socket_streambuf::ssl_started() const
	{
		return m_device->ssl_started();
	}

	bool asio_socket_streambuf::start_ssl()
	{
		if (ssl_started())
			return false;

		boost::asio::ssl::context ctx {boost::asio::ssl::context_base::sslv23_client};
		ctx.set_verify_mode(ctx.verify_none);
		return start_ssl(std::move(ctx));
	}

	bool asio_socket_streambuf::start_ssl(boost::asio::ssl::context ctx)
	{
		if (ssl_started())
			return false;

		BOOST_ASSERT(pptr() == pbase());
		m_device->start_ssl(std::move(ctx));
		return !m_device->last_error();
	}

	bool asio_socket_streambuf::stop_ssl()
	{
		if (!ssl_started())
			return false;

		if (sync() == -1)
		{
			m_device->close();
			return false;
		}

		m_device->stop_ssl();
		if (m_device->last_error())
		{
			m_device->close();
			return false;
		}

		return true;
	}
#endif

	bool asio_socket_streambuf::shutdown()
	{
		if (!is_open())
			return false;

		if (sync() == -1)
		{
			m_device->close();
			return false;
		}

		m_device->shutdown();

		if (m_device->last_error())
		{
			m_device->close();
			return false;
		}

		return true;
	}

	bool asio_socket_streambuf::close()
	{
		if (!is_open())
			return true;

		bool res = true;
#ifdef EXT_ENABLE_OPENSSL
		if (ssl_started())
			res &= stop_ssl();
#endif

		res = res && shutdown();
		m_device->close();
		return res;
	}

	void asio_socket_streambuf::interrupt()
	{
		m_device->interrupt();
	}

	/************************************************************************/
	/*               data operations                                        */
	/************************************************************************/
	std::streamsize asio_socket_streambuf::showmanyc()
	{
#ifdef EXT_ENABLE_OPENSSL
		// for encrypted we can't precisely tell how much available
		if (ssl_started())
			return 0;
#endif

		// on error available will return 0
		boost::system::error_code ignored;
		return m_device->socket().available(ignored);
	}

	std::size_t asio_socket_streambuf::read_some(char_type * data, std::size_t count)
	{
		return m_device->read_some(data, count);
	}

	std::size_t asio_socket_streambuf::write_some(const char_type * data, std::size_t count)
	{
		return m_device->write_some(data, count);
	}

	/************************************************************************/
	/*           setters/getters                                            */
	/************************************************************************/

	asio_socket_streambuf::duration_type asio_socket_streambuf::timeout() const
	{
		return m_device->timeout();
	}

	asio_socket_streambuf::duration_type asio_socket_streambuf::timeout(duration_type newtimeout)
	{
		return m_device->timeout(newtimeout);
	}

	const asio_socket_streambuf::error_code_type & asio_socket_streambuf::last_error() const
	{
		return m_device->last_error();
	}

	asio_socket_streambuf::error_code_type & asio_socket_streambuf::last_error()
	{
		return m_device->last_error();
	}

	boost::asio::ip::tcp::socket & asio_socket_streambuf::socket()
	{
		return m_device->socket();
	}

	std::shared_ptr<boost::asio::io_service> asio_socket_streambuf::service() const
	{
		return m_device->service();
	}

	void asio_socket_streambuf::peer_name(std::string & name, unsigned short & port)
	{
		if (!is_open())
			throw std::runtime_error("asio_socket_streambuf::peer_name: bad socket");
		
		auto & sock = m_device->socket();
		auto rep = sock.remote_endpoint();

		name = rep.address().to_string();
		port = rep.port();
	}

	auto asio_socket_streambuf::peer_name() -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		peer_name(res.first, res.second);
		return res;
	}

	std::string asio_socket_streambuf::peer_endpoint()
	{
		std::string address; unsigned short port;
		peer_name(address, port);
		return address + ':' + std::to_string(port);
	}

	std::string asio_socket_streambuf::peer_address()
	{
		std::string address; unsigned short port;
		peer_name(address, port);
		return address;
	}

	unsigned short asio_socket_streambuf::peer_port()
	{
		std::string address; unsigned short port;
		peer_name(address, port);
		return port;
	}

	void asio_socket_streambuf::sock_name(std::string & name, unsigned short & port)
	{
		if (!is_open())
			throw std::runtime_error("asio_socket_streambuf::sock_name: bad socket");
		
		auto & sock = m_device->socket();
		auto lep = sock.local_endpoint();
		
		name = lep.address().to_string();
		port = lep.port();
	}

	auto asio_socket_streambuf::sock_name() -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		sock_name(res.first, res.second);
		return res;
	}

	std::string asio_socket_streambuf::sock_endpoint()
	{
		std::string address; unsigned short port;
		sock_name(address, port);
		return address + ':' + std::to_string(port);
	}

	std::string asio_socket_streambuf::sock_address()
	{
		std::string address; unsigned short port;
		sock_name(address, port);
		return address;
	}

	unsigned short asio_socket_streambuf::sock_port()
	{
		std::string address; unsigned short port;
		sock_name(address, port);
		return port;
	}

	/************************************************************************/
	/*              ctors/dtors                                             */
	/************************************************************************/
	asio_socket_streambuf::asio_socket_streambuf()
	{
		auto ioserv = std::make_shared<boost::asio::io_service>();
		m_device = std::make_unique<device>(std::move(ioserv));
	}

	asio_socket_streambuf::asio_socket_streambuf(std::shared_ptr<boost::asio::io_service> ioserv)
	{
		m_device = std::make_unique<device>(std::move(ioserv));
	}

	asio_socket_streambuf::~asio_socket_streambuf() BOOST_NOEXCEPT
	{
		//TODO: can it throw ?
		close();
		m_device.reset();
	}

	asio_socket_streambuf::asio_socket_streambuf(asio_socket_streambuf && op) BOOST_NOEXCEPT
		: base_type(std::move(op)),
		  m_device(std::move(op.m_device))
	{

	}

	asio_socket_streambuf & asio_socket_streambuf::operator =(asio_socket_streambuf && op) BOOST_NOEXCEPT
	{
		if (this != &op)
		{
			base_type::operator=(std::move(op));
			m_device = std::move(op.m_device);
		}

		return *this;
	}

	void asio_socket_streambuf::swap(asio_socket_streambuf & op) BOOST_NOEXCEPT
	{
		using std::swap;
		base_type::swap(op);
		swap(m_device, op.m_device);
	}
}
