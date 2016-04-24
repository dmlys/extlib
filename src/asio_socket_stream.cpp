#include <ext/iostreams/asio_socket_stream.hpp>

namespace ext
{
	void asio_socket_stream::connect(const std::string & host, unsigned short port)
	{
		if (m_streambuf.connect(host, port))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void asio_socket_stream::connect(const std::string & host, const std::string & service)
	{
		if (m_streambuf.connect(host, service))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void asio_socket_stream::connect(const boost::asio::ip::tcp::endpoint & ep)
	{
		if (m_streambuf.connect(ep))
			clear();
		else
			setstate(std::ios::failbit);
	}

#ifdef EXT_ENABLE_OPENSSL
	void asio_socket_stream::start_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl())
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void asio_socket_stream::start_ssl(boost::asio::ssl::context ctx)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(std::move(ctx)))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void asio_socket_stream::stop_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.stop_ssl())
			setstate(std::ios::failbit | std::ios::badbit);
	}
#endif

	void asio_socket_stream::shutdown()
	{
		if (fail()) return;

		if (!m_streambuf.shutdown())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void asio_socket_stream::close()
	{
		if (fail()) return;

		if (!m_streambuf.close())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void asio_socket_stream::interrupt()
	{
		m_streambuf.interrupt();
	}

	void asio_socket_stream::reset()
	{
		m_streambuf.close();
		clear(std::ios::goodbit);
	}

	asio_socket_stream::asio_socket_stream()
		: std::iostream(&m_streambuf)
	{

	}

	asio_socket_stream::asio_socket_stream(const std::string & host, unsigned short port)
		: std::iostream(&m_streambuf)
	{
		connect(host, port);
	}

	asio_socket_stream::asio_socket_stream(const std::string & host, const std::string & service)
		: std::iostream(&m_streambuf)
	{
		connect(host, service);
	}

	asio_socket_stream::asio_socket_stream(const boost::asio::ip::tcp::endpoint & ep)
		: std::iostream(&m_streambuf)
	{
		connect(ep);
	}

	asio_socket_stream::asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv)
		: std::iostream(&m_streambuf), m_streambuf(std::move(ioserv))
	{

	}

	asio_socket_stream::asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv,
	                             const std::string & host, const std::string & service)
		: std::iostream(&m_streambuf), m_streambuf(std::move(ioserv))
	{
		connect(host, service);
	}

	asio_socket_stream::asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv,
	                             const std::string & host, unsigned short port)
		: std::iostream(&m_streambuf), m_streambuf(std::move(ioserv))
	{
		connect(host, port);
	}

	asio_socket_stream::asio_socket_stream(std::shared_ptr<boost::asio::io_service> ioserv,
	                             const boost::asio::ip::tcp::endpoint & ep)
		: std::iostream(&m_streambuf), m_streambuf(std::move(ioserv))
	{
		connect(ep);
	}

	asio_socket_stream::asio_socket_stream(asio_socket_stream && op) BOOST_NOEXCEPT
		: std::iostream(std::move(op)),
		  m_streambuf(std::move(op.m_streambuf))
	{

	}

	asio_socket_stream & asio_socket_stream::operator =(asio_socket_stream && op) BOOST_NOEXCEPT
	{
		if (this != &op)
		{
			this->std::iostream::operator= (std::move(op));
			m_streambuf = std::move(op.m_streambuf);
		}

		return *this;
	}

	void asio_socket_stream::swap(asio_socket_stream & op) BOOST_NOEXCEPT
	{
		this->std::iostream::swap(op);
		m_streambuf.swap(op.m_streambuf);
	}
}