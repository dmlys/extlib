// author: Dmitry Lysachenko
// date: Saturday 20 april 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <ext/iostreams/bsdsock_stream.hpp>

namespace ext
{
	void bsdsock_stream::connect(const std::string & host, unsigned short port)
	{
		if (m_streambuf.connect(host, port))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void bsdsock_stream::connect(const std::string & host, const std::string & service)
	{
		if (m_streambuf.connect(host, service))
			clear();
		else
			setstate(std::ios::failbit);
	}

#ifdef EXT_ENABLE_OPENSSL
	void bsdsock_stream::start_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl())
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void bsdsock_stream::start_ssl(SSL_CTX * sslctx)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslctx))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void bsdsock_stream::start_ssl(const SSL_METHOD * sslmethod)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslmethod))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void bsdsock_stream::start_ssl(const std::string & servername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(servername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void bsdsock_stream::start_ssl(const SSL_METHOD * sslmethod, const std::string & servername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslmethod, servername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void bsdsock_stream::stop_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.stop_ssl())
			setstate(std::ios::failbit | std::ios::badbit);
	}
#endif

	void bsdsock_stream::shutdown()
	{
		if (fail()) return;

		if (!m_streambuf.shutdown())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void bsdsock_stream::close()
	{
		if (fail()) return;

		if (!m_streambuf.close())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void bsdsock_stream::interrupt()
	{
		m_streambuf.interrupt();
	}

	void bsdsock_stream::reset()
	{
		m_streambuf.close();
		clear(std::ios::goodbit);
	}

	bsdsock_stream::bsdsock_stream()
		: std::iostream(&m_streambuf)
	{

	}

	bsdsock_stream::bsdsock_stream(socket_handle_type sock_handle)
		: std::iostream(&m_streambuf), m_streambuf(sock_handle)
	{

	}

	bsdsock_stream::bsdsock_stream(bsdsock_streambuf && buf)
		: std::iostream(&m_streambuf), m_streambuf(std::move(buf))
	{

	}

	bsdsock_stream::bsdsock_stream(const std::string & host, unsigned short port)
		: std::iostream(&m_streambuf)
	{
		connect(host, port);
	}

	bsdsock_stream::bsdsock_stream(const std::string & host, const std::string & service)
		: std::iostream(&m_streambuf)
	{
		connect(host, service);
	}

	bsdsock_stream::bsdsock_stream(bsdsock_stream && op) noexcept
		: std::iostream(std::move(op)),
		  m_streambuf(std::move(op.m_streambuf))
	{
		set_rdbuf(&m_streambuf);
	}

	bsdsock_stream & bsdsock_stream::operator =(bsdsock_stream && op) noexcept
	{
		if (this != &op)
		{
			this->std::iostream::operator= (std::move(op));
			m_streambuf = std::move(op.m_streambuf);
		}

		return *this;
	}

	void bsdsock_stream::swap(bsdsock_stream & op) noexcept
	{
		this->std::iostream::swap(op);
		m_streambuf.swap(op.m_streambuf);
	}
}
