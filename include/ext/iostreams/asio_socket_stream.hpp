#pragma once
#include <ostream>
#include <istream>
#include <ext/iostreams/asio_socket_streambuf.hpp>

namespace ext
{
	/// реализация iostream для сокета.
	/// подробнее смотри socket_streambuf
	class asio_socket_stream : public std::iostream
	{
		ext::asio_socket_streambuf m_streambuf;

	public:
		typedef ext::asio_socket_streambuf::error_code_type   error_code_type;
		typedef ext::asio_socket_streambuf::system_error_type system_error_type;
		typedef ext::asio_socket_streambuf::duration_type     duration_type;
		
	public:
		bool self_tie() const { return m_streambuf.self_tie(); }
		bool self_tie(bool tie) { return m_streambuf.self_tie(tie); }
		
		duration_type timeout() const                   { return m_streambuf.timeout();  }
		duration_type timeout(duration_type newtimeout) { return m_streambuf.timeout(newtimeout); }
		
		const boost::system::error_code & last_error() const { return m_streambuf.last_error(); }
		      boost::system::error_code & last_error()       { return m_streambuf.last_error(); }

		asio_socket_streambuf * rdbuf() { return &m_streambuf; }
		std::shared_ptr<boost::asio::io_service> service() const { return m_streambuf.service(); }
		boost::asio::ip::tcp::socket & socket() { return m_streambuf.socket(); }

		std::string peer_endpoint() { return m_streambuf.peer_endpoint(); }
		void peer_name(std::string & name, unsigned short & port) { return m_streambuf.peer_name(name, port); }
		auto peer_name() -> std::pair<std::string, unsigned short> { return m_streambuf.peer_name(); }
		std::string peer_address() { return m_streambuf.peer_address(); }
		unsigned short peer_port() { return m_streambuf.peer_port(); }

		std::string sock_endpoint() { return m_streambuf.sock_endpoint(); }
		void sock_name(std::string & name, unsigned short & port) { return m_streambuf.sock_name(name, port); }
		auto sock_name() -> std::pair<std::string, unsigned short> { return m_streambuf.sock_name(); }
		std::string sock_address() { return m_streambuf.sock_address(); }
		unsigned short sock_port() { return m_streambuf.sock_port(); }


		/// подключение открыто, если была попытка подключения, даже если не успешная.
		/// если resolve завершился неудачей - класс не считается открытым
		bool is_open() const { return m_streambuf.is_open(); }
		/// выоляет подключение rdbuf()->connect(host, port);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::string & host, unsigned short port);
		/// выоляет подключение rdbuf()->connect(host, service);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::string & host, const std::string & service);
		/// выоляет подключение rdbuf()->connect(host, service);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const boost::asio::ip::tcp::endpoint & ep);

#ifdef EXT_ENABLE_OPENSSL
		bool ssl_started() const { return m_streambuf.ssl_started(); }
		/// rdbuf()->start_ssl()
		/// в случае ошибки устанавливает failbit | badbit
		void start_ssl();
		/// rdbuf()->start_ssl(ctx)
		/// в случае ошибки устанавливает failbit | badbit
		void start_ssl(boost::asio::ssl::context ctx);
		/// rdbuf()->stop_ssl()
		/// в случае ошибки устанавливает failbit | badbit
		void stop_ssl();
#endif

		/// rdbuf()->studown()
		/// в случае ошибки устанавливает failbit | badbit
		void shutdown();
		/// rdbuf()->close()
		/// в случае ошибки устанавливает failbit | badbit
		void close();
		/// rdbuf()->interrupt()
		void interrupt();
		/// закрывает rdbuf()->close
		/// clear(std::ios::goodbit)
		void reset();

	public:
		asio_socket_stream();
		asio_socket_stream(const std::string & host, unsigned short port);
		asio_socket_stream(const std::string & host, const std::string & service);
		asio_socket_stream(const boost::asio::ip::tcp::endpoint & ep);

		asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv);
		asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv, const std::string & host, unsigned short port);
		asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv, const std::string & host, const std::string & service);
		asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv, const boost::asio::ip::tcp::endpoint & ep);

		asio_socket_stream(const asio_socket_stream &) = delete;
		asio_socket_stream & operator =(const asio_socket_stream &) = delete;

		asio_socket_stream(asio_socket_stream && right) BOOST_NOEXCEPT;
		asio_socket_stream & operator =(asio_socket_stream && right) BOOST_NOEXCEPT;
		void swap(asio_socket_stream & right) BOOST_NOEXCEPT;
	};

	inline void swap(asio_socket_stream & s1, asio_socket_stream & s2) BOOST_NOEXCEPT
	{
		s1.swap(s2);
	}
}