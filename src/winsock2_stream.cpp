// author: Dmitry Lysachenko
// date: Saturday 30 august 2015
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <ext/iostreams/winsock2_stream.hpp>

namespace ext
{
	void winsock2_stream::connect(const std::wstring & host, unsigned short port)
	{
		if (m_streambuf.connect(host, port))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void winsock2_stream::connect(const std::wstring & host, const std::wstring & service)
	{
		if (m_streambuf.connect(host, service))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void winsock2_stream::connect(const std::string & host, unsigned short port)
	{
		if (m_streambuf.connect(host, port))
			clear();
		else
			setstate(std::ios::failbit);
	}

	void winsock2_stream::connect(const std::string & host, const std::string & service)
	{
		if (m_streambuf.connect(host, service))
			clear();
		else
			setstate(std::ios::failbit);
	}

#ifdef EXT_ENABLE_OPENSSL
	void winsock2_stream::start_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl())
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(SSL_CTX * sslctx)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslctx))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(const SSL_METHOD * sslmethod)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslmethod))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(const std::string & servername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(servername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(const std::wstring & wservername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(wservername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(const SSL_METHOD * sslmethod, const std::string & servername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslmethod, servername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::start_ssl(const SSL_METHOD * sslmethod, const std::wstring & wservername)
	{
		if (fail()) return;

		if (!m_streambuf.start_ssl(sslmethod, wservername))
			setstate(std::ios::badbit | std::ios::failbit);
	}

	void winsock2_stream::stop_ssl()
	{
		if (fail()) return;

		if (!m_streambuf.stop_ssl())
			setstate(std::ios::failbit | std::ios::badbit);
	}
#endif

	void winsock2_stream::shutdown()
	{
		if (fail()) return;

		if (!m_streambuf.shutdown())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void winsock2_stream::close()
	{
		if (fail()) return;

		if (!m_streambuf.close())
			setstate(std::ios::failbit | std::ios::badbit);
	}

	void winsock2_stream::interrupt()
	{
		m_streambuf.interrupt();
	}

	void winsock2_stream::reset()
	{
		m_streambuf.close();
		clear(std::ios::goodbit);
	}

	winsock2_stream::winsock2_stream()
		: std::iostream(&m_streambuf)
	{

	}

	winsock2_stream::winsock2_stream(const std::wstring & host, unsigned short port)
		: std::iostream(&m_streambuf)
	{
		connect(host, port);
	}

	winsock2_stream::winsock2_stream(const std::wstring & host, const std::wstring & service)
		: std::iostream(&m_streambuf)
	{
		connect(host, service);
	}

	winsock2_stream::winsock2_stream(const std::string & host, unsigned short port)
		: std::iostream(&m_streambuf)
	{
		connect(host, port);
	}

	winsock2_stream::winsock2_stream(const std::string & host, const std::string & service)
		: std::iostream(&m_streambuf)
	{
		connect(host, service);
	}

	winsock2_stream::winsock2_stream(winsock2_stream && op) BOOST_NOEXCEPT
		: std::iostream(std::move(op)),
		  m_streambuf(std::move(op.m_streambuf))
	{
		set_rdbuf(&m_streambuf);
	};

	winsock2_stream & winsock2_stream::operator =(winsock2_stream && op) BOOST_NOEXCEPT
	{
		if (this != &op)
		{
			this->std::iostream::operator= (std::move(op));
			m_streambuf = std::move(op.m_streambuf);
		}

		return *this;
	}

	void winsock2_stream::swap(winsock2_stream & op) BOOST_NOEXCEPT
	{
		this->std::iostream::swap(op);
		m_streambuf.swap(op.m_streambuf);
	}
}
