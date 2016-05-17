// author: Dmitry Lysachenko
// date: Saturday 20 april 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <algorithm> // for std::max

#include <ext/itoa.hpp>
#include <ext/config.hpp>  // for EXT_UNREACHABLE
#include <ext/Errors.hpp>  // for ext::FormatError

#include <ext/iostreams/bsdsock_streambuf.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>  // for socket types
#include <sys/time.h>   // for struct timeval
#include <sys/socket.h> // for socket functions
#include <sys/select.h> /* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/ioctl.h>  // for ioctl
#include <arpa/inet.h>  // for inet_ntop/inet_pton
#include <netdb.h>      // for getaddrinfo/freeaddrinfo

#ifdef EXT_ENABLE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ext/openssl.hpp>
#endif // EXT_ENABLE_OPENSSL

namespace ext
{
	/************************************************************************/
	/*                auxiliary functions                                   */
	/************************************************************************/
	void bsdsock_stream_init()
	{
#ifdef EXT_ENABLE_OPENSSL
		ext::openssl_init();
#endif
	}

	struct gai_error_category_impl : public std::error_category
	{
		const char * name() const BOOST_NOEXCEPT override { return "gai"; }
		std::string message(int code) const override      { return ::gai_strerror(code); }
	};

	const gai_error_category_impl gai_error_category_instance;

	const std::error_category & gai_error_category()
	{
		return gai_error_category_instance;
	}

	BOOST_NORETURN void throw_bsdsock_error(int code, const char * errmsg)
	{
		throw bsdsock_streambuf::system_error_type(
			bsdsock_streambuf::error_code_type(code, std::generic_category()), errmsg
		);
	}

	BOOST_NORETURN void throw_bsdsock_error(int code, const std::string & errmsg)
	{
		throw bsdsock_streambuf::system_error_type(
			bsdsock_streambuf::error_code_type(code, std::generic_category()), errmsg
		);
	}

	void inet_ntop(const sockaddr * addr, std::string & str, unsigned short & port)
	{
		const char * res;
		const socklen_t buflen = INET6_ADDRSTRLEN;
		char buffer[buflen];

		if (addr->sa_family == AF_INET)
		{
			auto * addr4 = reinterpret_cast<const sockaddr_in *>(addr);
			res = ::inet_ntop(AF_INET, const_cast<in_addr *>(&addr4->sin_addr), buffer, buflen);
			port = ntohs(addr4->sin_port);
		}
		else if (addr->sa_family == AF_INET6)
		{
			auto * addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
			res = ::inet_ntop(AF_INET6, const_cast<in6_addr *>(&addr6->sin6_addr), buffer, buflen);
			port = ntohs(addr6->sin6_port);
		}
		else
		{
			throw bsdsock_streambuf::system_error_type(
				std::make_error_code(std::errc::address_family_not_supported),
				"inet_ntop unsupported address family"
			);
		}
		
		if (res == nullptr)
			throw_bsdsock_error(errno, "inet_ntop failed");

		str = res;
	}

	auto inet_ntop(const sockaddr * addr) -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		inet_ntop(addr, res.first, res.second);
		return res;
	}

	bool inet_pton(int family, const char * addr, sockaddr * out)
	{
		int res;
		if (family == AF_INET)
		{
			auto * addr4 = reinterpret_cast<sockaddr_in *>(out);
			res = ::inet_pton(family, addr, &addr4->sin_addr);
		}
		else if (family == AF_INET6)
		{
			auto * addr6 = reinterpret_cast<sockaddr_in6 *>(out);
			res = ::inet_pton(family, addr, &addr6->sin6_addr);
		}
		else
		{
			throw bsdsock_streambuf::system_error_type(
				std::make_error_code(std::errc::address_family_not_supported),
				"inet_pton unsupported address family"
			);
		}

		if (res == -1) throw_bsdsock_error(errno, "InetPtonW failed");
		return res > 0;
	}

	bool inet_pton(int family, const std::string & addr, sockaddr * out)
	{
		return inet_pton(family, addr.c_str(), out);
	}

	void addrinfo_deleter::operator ()(addrinfo_type * ptr) const
	{
		::freeaddrinfo(ptr);
	}

	addrinfo_ptr getaddrinfo(const char * host, const char * service)
	{
		std::error_code err;
		auto result = getaddrinfo(host, service, err);
		if (result) return result;

		throw bsdsock_streambuf::system_error_type(err, "getaddrinfo failed");
	}

	addrinfo_ptr getaddrinfo(const char * host, const char * service, std::error_code & err)
	{
		addrinfo_type hints;

		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		int res;
		addrinfo_type * ptr;
		do res = ::getaddrinfo(host, service, &hints, &ptr); while (res == EAI_AGAIN);
		if (res == 0)
		{
			err.clear();
			return addrinfo_ptr(ptr);
		}

		if (res == EAI_SYSTEM)
		{
			err.assign(errno, std::generic_category());
			return addrinfo_ptr(nullptr);
		}
		else
		{
			err.assign(res, gai_error_category());
			return addrinfo_ptr(nullptr);
		}
	}

	/************************************************************************/
	/*                bsdsock_streambuf                                */
	/************************************************************************/
	static inline void make_timeval(timeval & tv, std::chrono::system_clock::duration val)
	{
		long micro = std::chrono::duration_cast<std::chrono::microseconds>(val).count();
		tv.tv_sec = micro / 1000000;
		tv.tv_usec = micro % 1000000;
	}

	static void set_port(addrinfo_type * addr, unsigned short port)
	{
		static_assert(offsetof(sockaddr_in, sin_port) == offsetof(sockaddr_in6, sin6_port), "sin_port/sin6_port offset differs");
		for (; addr; addr = addr->ai_next)
		{
			reinterpret_cast<sockaddr_in *>(addr->ai_addr)->sin_port = htons(port);
		}
	}

	/************************************************************************/
	/*                   connect/resolve helpers                            */
	/************************************************************************/
	bool bsdsock_streambuf::do_resolve(const char * host, const char * service, addrinfo_type ** result)
	{
		addrinfo_type hints;
		
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		int res;
		do res = ::getaddrinfo(host, service, &hints, result); while (res == EAI_AGAIN);
		if (res == 0) return true;

		if (res == EAI_SYSTEM)
			m_lasterror.assign(errno, std::generic_category());
		else
			m_lasterror.assign(res, gai_error_category());
		
		return false;
	}
	
	bool bsdsock_streambuf::do_socktimeouts(handle_type sock)
	{
		int res;
		/// timeout для blocking операций
		struct timeval tv;
		make_timeval(tv, m_timeout);

		res = ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (res != 0) goto sockerror;
		res = ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		if (res != 0) goto sockerror;

		return true;

	sockerror:
		m_lasterror.assign(errno, std::generic_category());
		return false;
	}

	bool bsdsock_streambuf::do_createsocket(handle_type & sock, const addrinfo_type * addr)
	{
		sock = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (sock != -1) return true;

		m_lasterror.assign(errno, std::generic_category());
		return false;
	}

	inline bool bsdsock_streambuf::do_sockshutdown(handle_type sock, int how)
	{
		auto res = ::shutdown(sock, how);
		if (res == 0) return true;

		m_lasterror.assign(errno, std::generic_category());
		return false;
	}

	bool bsdsock_streambuf::do_sockclose(handle_type sock)
	{
		auto res = ::close(sock);
		if (res == 0) return true;

		m_lasterror.assign(errno, std::generic_category());
		return false;
	}

	bool bsdsock_streambuf::do_sockconnect(handle_type sock, addrinfo_type * addr, unsigned short port)
	{
		auto * in = reinterpret_cast<sockaddr_in *>(addr->ai_addr);
		in->sin_port = htons(port);
		return do_sockconnect(sock, addr);
	}
	
	bool bsdsock_streambuf::do_sockconnect(handle_type sock, const addrinfo_type * addr)
	{
		int err, res;
		socklen_t solen;
		StateType prevstate;
		bool closepipe, pubres; // в должны ли мы закрыть pipe, или он был закрыт в interrupt
		int pipefd[2]; // self pipe trick. [0] readable, [1] writable
		
		prevstate = Closed;
		m_lasterror.clear();

		res = ::pipe(pipefd);
		if (res != 0) goto sockerror;
		
		// что бы сделать timeout при подключении - устанавливаем не блокирующее поведение
		res = ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
		if (res != 0) goto sockerror;

		res = ::connect(sock, addr->ai_addr, addr->ai_addrlen);
		if (res == 0) goto connected; // connected immediately
		assert(res == -1);

		if ((err = errno) != EINPROGRESS)
			goto error;

		pubres = publish_connecting(pipefd[1]);
		if (!pubres) goto intrreq;

	again:
		struct timeval timeout;
		make_timeval(timeout, m_timeout);

		fd_set write_set, read_set;
		FD_ZERO(&write_set);
		FD_SET(sock, &write_set);
		FD_ZERO(&read_set);
		FD_SET(pipefd[0], &read_set);
		
		prevstate = Connecting;
		res = ::select(std::max(sock, pipefd[0]) + 1, &read_set, &write_set, nullptr, &timeout);
		if (res == 0) // timeout
		{
			err = ETIMEDOUT;
			goto error;
		}

		if (res == -1) goto sockerror;
		assert(res >= 1);
		
		solen = sizeof(err);
		res = ::getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &solen);
		if (res != 0) goto sockerror;
		if (err != 0) goto error;
		
	connected:
		pubres = publish_opened(sock, prevstate);
		if (!pubres) goto intrreq;

		// восстанавливаем blocking behavior
		res = ::fcntl(sock, F_SETFL, ::fcntl(sock, F_GETFL, 0) & ~O_NONBLOCK);
		if (res != 0) goto sockerror;

		m_lasterror.clear();
		return true;

	sockerror:
		err = errno;
	error:
		pubres = m_state.compare_exchange_strong(prevstate, Closed, std::memory_order_relaxed);
		// произошла ошибка, она может результатом closesocket из interrupt
		// если мы успешно перешли в Closed - interrupt'а не было, а значит это обычная ошибка
		if (pubres)
		{
			if (err == EINTR) goto again;
			m_lasterror.assign(err, std::generic_category());
			closepipe = true;
		}
		else intrreq:
		{
			// было прерывание, если оно было до publish_connecting, то мы должны закрыть pipefd[1]
			// иначе pipefd[1] был закрыт внутри interrupt
			m_lasterror = std::make_error_code(std::errc::interrupted);
			// такое возможно, только если мы не смогли опубликовать publish_connecting,
			closepipe = prevstate == Closed;
		}

		if (closepipe) ::close(pipefd[1]);
		::close(pipefd[0]);
		
		res = ::close(sock);
		assert(res == 0 || (res = errno) == 0);
		
		m_sockhandle = -1;
		return false;
	}
	
	bool bsdsock_streambuf::do_connect(const addrinfo_type * addr)
	{
		handle_type sock;
		for (; addr; addr = addr->ai_next)
		{
			// пытаемся создать сокет, если не получилось - это какая-то серьезная ошибка. man 3 socket.
			// исключение EPROTONOSUPPORT, не поддерживаемый protocol family, в этом случае пробуем следующий адрес
			bool res = do_createsocket(sock, addr);
			if (!res)
			{
				if (m_lasterror == std::errc::protocol_not_supported)
					continue;
				else
					return false;
			}

			// выставляем timeout'ы. если не получилось - все очень плохо
			res = do_socktimeouts(sock);
			if (!res)
			{
				::close(sock);
				return false;
			}

			// do_sockconnect публикует sock в m_sockhandle, а также учитывает interrupt сигналы.
			// в случае успеха:
			//   весь объект будет переведен в открытое состояние, вернет true
			// в случае ошибки или вызова interrupt:
			//   вернет false и гарантированно закроет sock, m_sockhandle == -1
			res = do_sockconnect(sock, addr);
			if (res)
			{
				init_buffers(); // from base_type
				return true;
			}

			// возможные коды ошибок для ::connect в man 3 connect
			// в случае ECONNREFUSED, ENETUNREACH, EHOSTUNREACH - имеет смысл попробовать след адрес
			
			auto & cat = m_lasterror.category();
			auto code = m_lasterror.value();

			bool try_next = cat == std::generic_category() &&
				(code == ECONNREFUSED || code == ENETUNREACH || code == EHOSTUNREACH);

			if (try_next)
				continue;
			else
				return false;
		}
		
		return false;
	}
	
	/************************************************************************/
	/*                     State methods                                    */
	/************************************************************************/
	bool bsdsock_streambuf::publish_connecting(handle_type sock)
	{
		StateType prev = Closed;
		m_sockhandle = sock;
		return m_state.compare_exchange_strong(prev, Connecting, std::memory_order_release);
	}

	inline bool bsdsock_streambuf::publish_opened(handle_type sock, StateType & expected)
	{
		// пытаемся переключится в Opened
		m_sockhandle = sock;
		return m_state.compare_exchange_strong(expected, Opened, std::memory_order_release);
	}

	bool bsdsock_streambuf::do_shutdown()
	{
		StateType prev = Opened;
		bool success = m_state.compare_exchange_strong(prev, Shutdowned, std::memory_order_relaxed);
		if (success) return do_sockshutdown(m_sockhandle, SHUT_RDWR);

		// не получилось, значит был запрос на прерывание и shutdown был вызван оттуда
		// или же мы уже сделали shutdown ранее
		assert(prev == Interrupted || prev == Shutdowned);
		if (prev == Interrupted) m_lasterror = std::make_error_code(std::errc::interrupted);
		return false;
	}

	bool bsdsock_streambuf::do_close()
	{
		StateType prev = m_state.exchange(Closed, std::memory_order_release);

#ifdef EXT_ENABLE_OPENSSL
		do_sslreset();
#endif //EXT_ENABLE_OPENSSL

		auto sock = m_sockhandle;
		m_sockhandle = -1;

		// если мы были в Interrupting, тогда вызов interrupt закроет сокет
		if (prev == Interrupting) return true;
		return sock == -1 ? true : do_sockclose(sock);
	}

	bool bsdsock_streambuf::shutdown()
	{
		if (!is_open())
			return false;

		// try flush
		if (sync() == -1) return false;

#ifdef EXT_ENABLE_OPENSSL
		// try stop ssl
		if (!stop_ssl()) return false;
#endif //EXT_ENABLE_OPENSSL

		// делаем shutdown
		return do_shutdown();
	}

	bool bsdsock_streambuf::close()
	{
		bool result;
		if (!is_open())
			result = do_close();
		else
		{
			// делаем shutdown, после чего в любом случае закрываем сокет
			result = shutdown();
			result &= do_close();
		}

		// если закрытие успешно, очищаем последнюю ошибку
		if (result) m_lasterror.clear();
		return result;
	}
	
	void bsdsock_streambuf::interrupt()
	{
		int res;
		StateType prev;
		handle_type sock;

		prev = m_state.load(std::memory_order_acquire);
		do switch(prev)
		{
			// если мы уже прерваны или прерываемся - просто выходим
			case Interrupted:
			case Interrupting: return;
			// иначе пытаемся перейти в Interrupting
			default:
				sock = m_sockhandle;

		} while (!m_state.compare_exchange_weak(prev, Interrupting, std::memory_order_acquire, std::memory_order_relaxed));
		
		// ok, перешли
		auto state = prev;
		prev = Interrupting;

		switch (state)
		{
			case Interrupting:
			case Interrupted:
			default:
				EXT_UNREACHABLE(); // сюда мы попасть не должны по определению

			case Closed:
			case Shutdowned:
				m_state.compare_exchange_strong(prev, Interrupted, std::memory_order_relaxed);
				return; // в данном состояние сокет не ожидается в блокирующих состояниях -
				        // никаких доп действий не требуется. Но состояние перекинуто в Interrupted.
			
			case Connecting:
				// состояние подключения, это значит что есть поток который внутри класса и он на некоей стадии подключения.
				// вместе с сокетом внутри do_connect был создан pipe. Он вместе с socket слушается в select.
				// что бы прервать нужно записать что-нибудь в pipe.
				assert(sock != -1);
				::write(sock, &prev, sizeof(prev));
				res = ::close(sock);
				assert(res == 0 || (res = errno) == 0);

				m_state.compare_exchange_strong(prev, Interrupted, std::memory_order_relaxed);
				return;
		
			case Opened:
				// нормальный режим работы, сокет открыт, и потенциально есть блокирующий вызов: recv/send
				assert(sock != -1);
				res = ::shutdown(sock, SHUT_RDWR);
				assert(res == 0);

				bool success = m_state.compare_exchange_strong(prev, Interrupted, std::memory_order_relaxed);
				if (success) return;

				// пока мы shutdown'лись, был запрос на закрытие класса из основного потока,
				// он же видя что мы в процессе interrupt'а не стал делать закрытие сокета, а оставил это нам
				assert(prev == Closed);
				assert(sock != -1);
				res = ::close(sock);
				assert(res == 0 || (res = errno) == 0);
				return;
		}
	}
	
	/************************************************************************/
	/*                   read/write/others                                  */
	/************************************************************************/
	bool bsdsock_streambuf::is_valid() const
	{
		return m_sockhandle != -1 && !m_lasterror;
	}

	bool bsdsock_streambuf::is_open() const
	{
		return m_sockhandle != -1;
	}

	std::streamsize bsdsock_streambuf::showmanyc()
	{
		//if (!is_valid()) return 0;

#ifdef EXT_ENABLE_OPENSSL
		if (ssl_started())
		{
			return SSL_pending(m_sslhandle);
		}
#endif //EXT_ENABLE_OPENSSL

		unsigned avail = 0;
		auto res = ::ioctl(m_sockhandle, FIONREAD, &avail);
		return res == 0 ? avail : 0;
	}

	std::size_t bsdsock_streambuf::read_some(char_type * data, std::size_t count)
	{
		//if (!is_valid()) return 0;

#ifdef EXT_ENABLE_OPENSSL
		if (ssl_started())
		{
		ssl_again:
			res = ::SSL_read(m_sslhandle, data, count);
			if (res > 0) return res;
			
			if (ssl_rw_error(res, m_lasterror)) goto ssl_again;
			return 0;
		}
#endif // EXT_ENABLE_OPENSSL

	again:
		int res = ::recv(m_sockhandle, data, count, 0);
		if (res >= 0) return res;
		
		if (rw_error(errno, m_lasterror)) goto again;
		return 0;
	}

	std::size_t bsdsock_streambuf::write_some(const char_type * data, std::size_t count)
	{
		//if (!is_valid()) return 0;

#ifdef EXT_ENABLE_OPENSSL
		if (ssl_started())
		{
		ssl_again:
			res = ::SSL_write(m_sslhandle, data, count);
			if (res > 0) return res;

			if (ssl_rw_error(res, m_lasterror)) goto ssl_again;
			return 0;
		}
#endif // EXT_ENABLE_OPENSSL

	again:
		int res = ::send(m_sockhandle, data, count, 0);
		if (res >= 0) return res;

		if (rw_error(errno, m_lasterror)) goto again;
		return 0;
	}
	
	bool bsdsock_streambuf::rw_error(int err, error_code_type & err_code)
	{
		// error can be result of shutdown from interrupt
		auto state = m_state.load(std::memory_order_relaxed);
		if (state >= Interrupting) 
		{
			err_code = std::make_error_code(std::errc::interrupted);
			return false;
		}
		
		if (err == EINTR) return true;
		
		// linux and probably other *nix 
		// returns EAGAIN if SO_RCVTIMEO timeout occurs
		if (err == EAGAIN || err == EWOULDBLOCK) err = ETIMEDOUT;
		
		err_code.assign(err, std::generic_category());
		return false;
	}
	
	/************************************************************************/
	/*                   actual connect                                     */
	/************************************************************************/
	bool bsdsock_streambuf::connect(const addrinfo_type & addr)
	{
		assert(&addr);
		if (is_open())
		{
			m_lasterror = std::make_error_code(std::errc::already_connected);
			return false;
		}
		
		return do_connect(&addr);
	}

	bool bsdsock_streambuf::connect(const std::string & host, const std::string & service)
	{
		if (is_open())
		{
			m_lasterror = std::make_error_code(std::errc::already_connected);
			return false;
		}

		addrinfo_type * addr = nullptr;
		bool res = do_resolve(host.c_str(), service.c_str(), &addr);
		if (!res) return false;

		assert(addr);
		res = do_connect(addr);

		::freeaddrinfo(addr);
		return res;
	}

	bool bsdsock_streambuf::connect(const std::string & host, unsigned short port)
	{
		if (is_open())
		{
			m_lasterror = std::make_error_code(std::errc::already_connected);
			return false;
		}

		addrinfo_type * addr = nullptr;
		bool res = do_resolve(host.c_str(), nullptr, &addr);
		if (!res) return false;
		
		set_port(addr, port);
		res = do_connect(addr);
		
		::freeaddrinfo(addr);
		return res;
	}

	/************************************************************************/
	/*                     ssl stuff                                        */
	/************************************************************************/
#ifdef EXT_ENABLE_OPENSSL
	bool bsdsock_streambuf::ssl_rw_error(int res, error_code_type & err_code)
	{
		// error can be result of shutdown from interrupt
		auto state = m_state.load(std::memory_order_relaxed);
		if (state >= Interrupting)
		{
			err_code = std::make_error_code(std::errc::interrupted);
			return false;
		}

		if (ssl_started())
		{
			res = SSL_get_error(m_sslhandle, res);
			int err;
			switch (res)
			{
				// can this happen? just try to handle as SSL_ERROR_SYSCALL
				case SSL_ERROR_NONE:

					// if it's SSL_ERROR_WANT_{WRITE,READ}
					// errno can be EAGAIN - then it's timeout, or EINTR - repeat operation
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_SYSCALL:
				case SSL_ERROR_SSL:
					// if it some generic SSL error
					if ((err = ERR_get_error()))
					{
						err_code.assign(err, ext::openssl_err_category());
						return false;
					}

					if ((err = errno))
					{
						if (err == EINTR) return true;

						// linux and probably other *nix 
						// returns EAGAIN if SO_RCVTIMEO timeout occurs
						if (err == EAGAIN || err == EWOULDBLOCK) err = ETIMEDOUT;

						err_code.assign(err, std::generic_category());
						return false;
					}

				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_WANT_X509_LOOKUP:
				case SSL_ERROR_WANT_CONNECT:
				case SSL_ERROR_WANT_ACCEPT:
				default:
					m_lasterror.assign(res, ext::openssl_err_category());
					return false;
			}
		}

		res = errno; // unused
		if (res == EINTR) return true;

		// linux and probably other *nix 
		// returns EAGAIN if SO_RCVTIMEO timeout occurs
		if (res == EAGAIN || res == EWOULDBLOCK) res = ETIMEDOUT;

		err_code.assign(res, std::generic_category());
		return false;
	}

	bool bsdsock_streambuf::ssl_started() const
	{
		return m_sslhandle != nullptr && SSL_get_session(m_sslhandle) != nullptr;
	}

	void bsdsock_streambuf::do_sslreset()
	{
		SSL_free(m_sslhandle);
		m_sslhandle = nullptr;
	}

	bsdsock_streambuf::error_code_type bsdsock_streambuf::ssl_error(SSL * ssl, int error)
	{
		int ssl_err = SSL_get_error(ssl, error);
		return ext::openssl_geterror(ssl_err);
	}

	bool bsdsock_streambuf::do_createssl(SSL *& ssl, SSL_CTX * sslctx)
	{
		ssl = SSL_new(sslctx);
		if (!ssl)
		{
			m_lasterror.assign(static_cast<int>(ERR_get_error()), ext::openssl_err_category());
			return false;
		}

		int res = SSL_set_fd(ssl, m_sockhandle);
		if (res <= 0) goto error;

		SSL_set_mode(ssl, SSL_get_mode(ssl) | SSL_MODE_AUTO_RETRY);
		return true;

	error:
		m_lasterror = ssl_error(ssl, res);
		SSL_free(ssl);
		ssl = nullptr;
		return false;
	}

	bool bsdsock_streambuf::do_sslconnect(SSL * ssl)
	{
	again:
		int res = SSL_connect(ssl);
		if (res > 0) return true;

		if (ssl_rw_error(res, m_lasterror)) goto again;
		return false;
	}

	bool bsdsock_streambuf::do_sslshutdown(SSL * ssl)
	{
		// смотри описание 2х фазного SSL_shutdown в описании функции SSL_shutdown:
		// https://www.openssl.org/docs/manmaster/ssl/SSL_shutdown.html
		int res = SSL_shutdown(ssl);
		if (res < 0) goto error;
		if (res == 0)
		{
			res = SSL_shutdown(ssl);
			if (res <= 0)
			{
				m_lasterror = ssl_error(ssl, res);
				// второй shutdown не получился, это может быть как ошибка,
				// так и нам просто закрыли канал по shutdown на другой стороне. проверяем

				handle_type sock = SSL_get_fd(ssl);
				fd_set rdset;
				FD_ZERO(&rdset);
				FD_SET(sock, &rdset);

				struct timeval tv = {0, 0};
				auto selres = select(0, &rdset, nullptr, nullptr, &tv);
				if (selres <= 0) return false;
				
				char c;
				auto rc = recv(sock, &c, 1, MSG_PEEK);
				if (rc != 0) return false; // socket closed

				// да мы действительно получили FD_CLOSE
				m_lasterror.clear();
			}
		}

		res = SSL_clear(ssl);
		if (res <= 0) goto error;

		return true;

	error:
		// -1 - error or nonblocking, we are always blocking
		m_lasterror = ssl_error(ssl, res);
		return false;
	}

	bool bsdsock_streambuf::start_ssl_weak(SSL_CTX * sslctx)
	{
		if (!is_open())
		{
			m_lasterror = std::make_error_code(std::errc::not_a_socket);
			return false;
		}

		if (ssl_started()) return true;
		do_sslreset();
		return do_createssl(m_sslhandle, sslctx) && do_sslconnect(m_sslhandle);
	}

	bool bsdsock_streambuf::start_ssl(SSL_CTX * sslctx)
	{
		auto res = start_ssl_weak(sslctx);
		SSL_CTX_free(sslctx);
		return res;
	}

	bool bsdsock_streambuf::start_ssl(const SSL_METHOD * sslmethod)
	{
		SSL_CTX * sslctx = SSL_CTX_new(sslmethod);
		if (sslctx == nullptr)
		{
			m_lasterror.assign(static_cast<int>(ERR_get_error()), ext::openssl_err_category());
			return false;
		}

		auto res = start_ssl_weak(sslctx);
		SSL_CTX_free(sslctx);
		return res;
	}

	bool bsdsock_streambuf::start_ssl()
	{
		if (m_sslhandle)
			return do_sslconnect(m_sslhandle);
		else
		{
			const SSL_METHOD * sslm = SSLv23_client_method();
			return start_ssl(sslm);
		}
	}

	bool bsdsock_streambuf::stop_ssl()
	{
		if (!ssl_started()) return true;

		// flush failed
		if (sync() == -1) return false;
		return do_sslshutdown(m_sslhandle);
	}

#endif //EXT_ENABLE_OPENSSL
	
	/************************************************************************/
	/*                     getters/setters                                  */
	/************************************************************************/
	bsdsock_streambuf::duration_type bsdsock_streambuf::timeout(duration_type newtimeout)
	{
		if (newtimeout < std::chrono::seconds(1))
			newtimeout = std::chrono::seconds(1);
		
		newtimeout = std::exchange(m_timeout, newtimeout);
		if (is_open()) do_socktimeouts(m_sockhandle);
		return newtimeout;
	}

	void bsdsock_streambuf::getpeername(sockaddr_type * addr, socklen_t * addrlen)
	{
		if (!is_open())
			throw std::runtime_error("bsdsock_streambuf::getpeername: bad socket");

		auto res = ::getpeername(m_sockhandle, addr, addrlen);
		if (res != 0)
		{
			throw system_error_type(
				error_code_type(errno, std::generic_category()),
				"bsdsock_streambuf::peer_name getpeername failed"
			);
		}
	}

	void bsdsock_streambuf::getsockname(sockaddr_type * addr, socklen_t * addrlen)
	{
		if (!is_open())
			throw std::runtime_error("bsdsock_streambuf::getsockname: bad socket");

		auto res = ::getsockname(m_sockhandle, addr, addrlen);
		if (res != 0)
		{
			throw std::system_error(
				error_code_type(errno, std::generic_category()),
				"bsdsock_streambuf::sock_name getsockname failed"
			);
		}
	}

	std::string bsdsock_streambuf::peer_endpoint()
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getpeername(addr, &addrlen);

		std::string host;
		unsigned short port;
		ext::inet_ntop(addr, host, port);
		
		char buffer[std::numeric_limits<unsigned short>::digits10 + 2];		
		host += ':';
		host += ext::itoa(port, buffer);
		
		return host;
	}

	std::string bsdsock_streambuf::sock_endpoint()
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getsockname(addr, &addrlen);

		std::string host;
		unsigned short port;
		ext::inet_ntop(addr, host, port);
		
		char buffer[std::numeric_limits<unsigned short>::digits10 + 2];
		host += ':';
		host += ext::itoa(port, buffer);
		
		return host;
	}

	unsigned short bsdsock_streambuf::peer_port()
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getpeername(addr, &addrlen);

		// both sockaddr_in6 and sockaddr_in have port member on same offset
		auto port = reinterpret_cast<sockaddr_in6 *>(addr)->sin6_port;
		return ntohs(port);
	}

	unsigned short bsdsock_streambuf::sock_port()
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getsockname(addr, &addrlen);

		// both sockaddr_in6 and sockaddr_in have port member on same offset
		auto port = reinterpret_cast<sockaddr_in6 *>(addr)->sin6_port;
		return ntohs(port);
	}

	void bsdsock_streambuf::peer_name(std::string & name, unsigned short & port)
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getpeername(addr, &addrlen);

		ext::inet_ntop(addr, name, port);
	}

	void bsdsock_streambuf::sock_name(std::string & name, unsigned short & port)
	{
		sockaddr_storage addrstore;
		socklen_t addrlen = sizeof(addrstore);
		auto * addr = reinterpret_cast<sockaddr *>(&addrstore);
		getsockname(addr, &addrlen);

		ext::inet_ntop(addr, name, port);
	}

	auto bsdsock_streambuf::peer_name() -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		peer_name(res.first, res.second);
		return res;
	}

	auto bsdsock_streambuf::sock_name() -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		sock_name(res.first, res.second);
		return res;
	}

	std::string bsdsock_streambuf::peer_address()
	{
		std::string addr; unsigned short port;
		peer_name(addr, port);
		return addr;
	}

	std::string bsdsock_streambuf::sock_address()
	{
		std::string addr; unsigned short port;
		sock_name(addr, port);
		return addr;
	}

	/************************************************************************/
	/*                   ctors/dtor                                         */
	/************************************************************************/
	bsdsock_streambuf::bsdsock_streambuf() BOOST_NOEXCEPT
	{
		m_sockhandle = -1;
	}

	bsdsock_streambuf::~bsdsock_streambuf() BOOST_NOEXCEPT
	{
		close();
	}

	bsdsock_streambuf::bsdsock_streambuf(bsdsock_streambuf && right) BOOST_NOEXCEPT
		: base_type(std::move(right)),
		  m_sockhandle(std::exchange(right.m_sockhandle, -1)),
#ifdef EXT_ENABLE_OPENSSL
		  m_sslhandle(std::exchange(right.m_sslhandle, nullptr)),
#endif
		  m_state(right.m_state.exchange(Closed, std::memory_order_relaxed)),
		  m_lasterror(std::exchange(right.m_lasterror, error_code_type {})),
		  m_timeout(right.m_timeout)
	{

	}

	bsdsock_streambuf & bsdsock_streambuf::operator=(bsdsock_streambuf && right) BOOST_NOEXCEPT
	{
		if (this != &right)
		{
			close();

			base_type::operator =(std::move(right));
			m_sockhandle = std::exchange(right.m_sockhandle, -1);
#ifdef EXT_ENABLE_OPENSSL
			m_sslhandle = std::exchange(right.m_sslhandle, nullptr);
#endif
			m_state.store(right.m_state.exchange(Closed, std::memory_order_relaxed), std::memory_order_relaxed);
			m_lasterror = std::exchange(right.m_lasterror, error_code_type {});
			m_timeout = right.m_timeout;
		}

		return *this;
	}

	void bsdsock_streambuf::swap(bsdsock_streambuf & other) BOOST_NOEXCEPT
	{
		using std::swap;

		auto tmp = std::move(other);
		other = std::move(*this);
		*this = std::move(tmp);
	}
	
} // namespace ext

