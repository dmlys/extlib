#pragma once
// author: Dmitry Lysachenko
// date: Saturday 30 august 2015
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <string>
#include <ostream>
#include <istream>
#include <ext/iostreams/winsock2_streambuf.hpp>

namespace ext
{
	/// реализация iostream для сокета.
	/// подробнее смотри winsock2_streambuf
	class winsock2_stream : public std::iostream
	{
		ext::winsock2_streambuf m_streambuf;

	public:
		typedef ext::winsock2_streambuf::error_code_type   error_code_type;
		typedef ext::winsock2_streambuf::system_error_type system_error_type;

		typedef ext::winsock2_streambuf::duration_type duration_type;
		typedef ext::winsock2_streambuf::time_point    time_point;

		typedef ext::winsock2_streambuf::handle_type   handle_type;

	public:
		/************************************************************************/
		/*                 forward some methods                                 */
		/************************************************************************/
		bool self_tie() const { return m_streambuf.self_tie(); }
		bool self_tie(bool tie) { return m_streambuf.self_tie(tie); }
		
		duration_type timeout() const                   { return m_streambuf.timeout();  }
		duration_type timeout(duration_type newtimeout) { return m_streambuf.timeout(newtimeout); }
		
		const error_code_type & last_error() const { return m_streambuf.last_error(); }
		      error_code_type & last_error()       { return m_streambuf.last_error(); }

		ext::winsock2_streambuf * rdbuf() { return &m_streambuf; }
		handle_type socket() { return m_streambuf.socket(); }

		void getpeername(sockaddr_type * addr, int * addrlen) { m_streambuf.getpeername(addr, addrlen); }
		void getsockname(sockaddr_type * addr, int * addrlen) { m_streambuf.getsockname(addr, addrlen); }

		std::string peer_endpoint() { return m_streambuf.peer_endpoint(); }
		void peer_name(std::string & name, unsigned short & port)  { return m_streambuf.peer_name(name, port); }
		auto peer_name() -> std::pair<std::string, unsigned short> { return m_streambuf.peer_name(); }
		std::string peer_address() { return m_streambuf.peer_address(); }
		unsigned short peer_port() { return m_streambuf.peer_port(); }

		std::string sock_endpoint() { return m_streambuf.sock_endpoint(); }
		void sock_name(std::string & name, unsigned short & port)  { return m_streambuf.sock_name(name, port); }
		auto sock_name() -> std::pair<std::string, unsigned short> { return m_streambuf.sock_name(); }
		std::string sock_address() { return m_streambuf.sock_address(); }
		unsigned short sock_port() { return m_streambuf.sock_port(); }


		/// подключение не валидно, если оно не открыто или была ошибка в процессе работы
		bool is_valid() const { return m_streambuf.is_valid(); }
		/// подключение открыто, если была попытка подключения, даже если не успешная.
		/// если resolve завершился неудачей - класс не считается открытым
		bool is_open() const { return m_streambuf.is_open(); }

		/// выоляет подключение rdbuf()->connect(host, port);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::wstring & host, unsigned short port);
		/// выоляет подключение rdbuf()->connect(host, service);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::wstring & host, const std::wstring & service);
		/// выоляет подключение rdbuf()->connect(host, port);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::string & host, unsigned short port);
		/// выоляет подключение rdbuf()->connect(host, service);
		/// в случае ошибки устанавливает failbit | badbit
		void connect(const std::string & host, const std::string & service);

#ifdef EXT_ENABLE_OPENSSL
		bool ssl_started() const { return m_streambuf.ssl_started(); }
		SSL * ssl_handle() { return m_streambuf.ssl_handle(); }
		void set_ssl(SSL * ssl) { return m_streambuf.set_ssl(ssl); }
		/// rdbuf()->start_ssl(...)
		/// в случае ошибки устанавливает failbit | badbit
		void start_ssl();
		void start_ssl(SSL_CTX * sslctx);
		void start_ssl(const SSL_METHOD * sslmethod);
		void start_ssl(const std::string & servername);
		void start_ssl(const std::wstring & wservername);
		void start_ssl(const SSL_METHOD * sslmethod, const std::string & servername);
		void start_ssl(const SSL_METHOD * sslmethod, const std::wstring & wservername);
		/// rdbuf()->stop_ssl()
		/// в случае ошибки устанавливает failbit | badbit
		void stop_ssl();
		/// rdbuf()->free_ssl();
		void free_ssl() { return m_streambuf.free_ssl(); }
#endif //EXT_ENABLE_OPENSSL

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
		winsock2_stream();
		winsock2_stream(const std::wstring & host, unsigned short port);
		winsock2_stream(const std::wstring & host, const std::wstring & service);
		winsock2_stream(const std::string & host, unsigned short port);
		winsock2_stream(const std::string & host, const std::string & service);

		winsock2_stream(const winsock2_stream &) = delete;
		winsock2_stream & operator =(const winsock2_stream &) = delete;

		winsock2_stream(winsock2_stream && right) noexcept;
		winsock2_stream & operator =(winsock2_stream && right) noexcept;
		void swap(winsock2_stream & right) noexcept;
	};

	inline void swap(winsock2_stream & s1, winsock2_stream & s2) noexcept
	{
		s1.swap(s2);
	}
}
