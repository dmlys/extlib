// author: Dmitry Lysachenko
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <codecvt>
#include <iostream>

#include <ext/iostreams/winsock2_inc.hpp>
#include <ext/iostreams/winsock2_streambuf.hpp>
#include <ext/codecvt_conv.hpp>
#include <ext/Errors.hpp>  // for ext::FormatError

#ifdef EXT_ENABLE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ext/openssl.hpp>
#endif // EXT_ENABLE_OPENSSL

#ifdef _MSC_VER
// warning C4244: '=' : conversion from '__int64' to 'long', possible loss of data
// warning C4244: 'initializing' : conversion from '__int64' to 'long', possible loss of data
#pragma warning(disable : 4267 4244)
#pragma comment(lib, "ws2_32.lib")

#endif // _MSC_VER


namespace ext
{
	int wsastartup(std::uint16_t version)
	{
		WSADATA wsadata;
		auto res = ::WSAStartup(version, &wsadata);
		return res;
	}

	void wsastartup()
	{
		WORD ver = MAKEWORD(2, 2);
		WSADATA wsadata;
		int res = ::WSAStartup(ver, &wsadata);
		if (res == 0) return;

		std::cerr
			<< "Failed to initialize winsock version 2.2 library. "
			<< ext::FormatError(std::error_code(res, std::system_category()))
			<< std::endl;

		std::exit(EXIT_FAILURE);
	}

	void wsacleanup()
	{
		::WSACleanup();
	}

	void winsock2_stream_init()
	{
		ext::wsastartup();

#ifdef EXT_ENABLE_OPENSSL
		ext::openssl_init();
#endif
	}

	int last_socket_error() noexcept
	{
		return ::WSAGetLastError();
	}

	std::error_code last_socket_error_code() noexcept
	{
		return std::error_code(::WSAGetLastError(), std::system_category());
	}

	BOOST_NORETURN void throw_socket_error(int code, const char * errmsg)
	{
		throw winsock2_streambuf::system_error_type(
			winsock2_streambuf::error_code_type(code, std::system_category()), errmsg
		);
	}

	BOOST_NORETURN void throw_socket_error(int code, const std::string & errmsg)
	{
		throw winsock2_streambuf::system_error_type(
			winsock2_streambuf::error_code_type(code, std::system_category()), errmsg
		);
	}

	BOOST_NORETURN void throw_last_socket_error(const std::string & errmsg)
	{
		throw winsock2_streambuf::system_error_type(last_socket_error_code(), errmsg);
	}

	void inet_ntop(const sockaddr * addr, std::wstring & wstr, unsigned short & port)
	{
		const wchar_t * res;
		DWORD buflen = INET6_ADDRSTRLEN;
		wchar_t buffer[INET6_ADDRSTRLEN];

		if (addr->sa_family == AF_INET)
		{
			auto * addr4 = reinterpret_cast<const sockaddr_in *>(addr);
			res = ::InetNtopW(AF_INET, const_cast<in_addr *>(&addr4->sin_addr), buffer, buflen);
			port = ::ntohs(addr4->sin_port);
		}
		else if (addr->sa_family == AF_INET6)
		{
			auto * addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
			res = InetNtopW(AF_INET6, const_cast<in_addr6 *>(&addr6->sin6_addr), buffer, buflen);
			port = ::ntohs(addr6->sin6_port);
		}
		else
		{
			throw winsock2_streambuf::system_error_type(
				std::make_error_code(std::errc::address_family_not_supported),
				"inet_ntop unsupported address family"
				);
		}

		if (res == nullptr)
			throw_last_socket_error("InetNtopW failed");

		wstr.assign(res);
	}

	void inet_ntop(const sockaddr * addr, std::string & str, unsigned short & port)
	{
		const wchar_t * res;
		DWORD buflen = INET6_ADDRSTRLEN;
		wchar_t buffer[INET6_ADDRSTRLEN];

		if (addr->sa_family == AF_INET)
		{
			auto * addr4 = reinterpret_cast<const sockaddr_in *>(addr);
			res = ::InetNtopW(AF_INET, const_cast<in_addr *>(&addr4->sin_addr), buffer, buflen);
			port = ::ntohs(addr4->sin_port);
		}
		else if (addr->sa_family == AF_INET6)
		{
			auto * addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
			res = InetNtopW(AF_INET6, const_cast<in_addr6 *>(&addr6->sin6_addr), buffer, buflen);
			port = ::ntohs(addr6->sin6_port);
		}
		else
		{
			throw winsock2_streambuf::system_error_type(
				std::make_error_code(std::errc::address_family_not_supported),
				"inet_ntop unsupported address family"
				);
		}

		if (res == nullptr)
			throw_last_socket_error("InetNtopW failed");

		std::codecvt_utf8<wchar_t> cvt;
		auto in = boost::make_iterator_range_n(buffer, std::wcslen(buffer));
		ext::codecvt_convert::to_bytes(cvt, in, str);
	}

	auto inet_ntop(const sockaddr * addr) -> std::pair<std::string, unsigned short>
	{
		std::pair<std::string, unsigned short> res;
		inet_ntop(addr, res.first, res.second);
		return res;
	}



	bool inet_pton(int family, const wchar_t * waddr, sockaddr * out)
	{
		INT res;
		if (family == AF_INET)
		{
			auto * addr4 = reinterpret_cast<sockaddr_in *>(out);
			res = ::InetPtonW(family, waddr, &addr4->sin_addr);
		}
		else if (family == AF_INET6)
		{
			auto * addr6 = reinterpret_cast<sockaddr_in6 *>(out);
			res = ::InetPtonW(family, waddr, &addr6->sin6_addr);
		}
		else
		{
			throw_socket_error(WSAEAFNOSUPPORT, "InetPtonW failed");
		}
		
		if (res == -1) throw_last_socket_error("InetPtonW failed");
		return res > 0;
	}

	bool inet_pton(int family, const std::wstring & waddr, sockaddr * out)
	{
		INT res = InetPtonW(family, waddr.c_str(), out);
		if (res == -1) throw_last_socket_error("InetPtonW failed");
		return res > 0;
	}

	bool inet_pton(int family, const char * addr, sockaddr * out)
	{
		std::codecvt_utf8<wchar_t> cvt;
		auto in = boost::make_iterator_range_n(addr, std::strlen(addr));
		auto waddr = ext::codecvt_convert::from_bytes(cvt, in);

		return inet_pton(family, waddr.c_str(), out);
	}

	bool inet_pton(int family, const std::string & addr, sockaddr * out)
	{
		std::codecvt_utf8<wchar_t> cvt;
		auto waddr = ext::codecvt_convert::from_bytes(cvt, addr);

		return inet_pton(family, waddr.c_str(), out);
	}

	/************************************************************************/
	/*                   getaddrinfo                                        */
	/************************************************************************/
	void addrinfo_deleter::operator ()(addrinfo_type * ptr) const
	{
		FreeAddrInfoW(ptr);
	}

	addrinfo_ptr getaddrinfo(const wchar_t * host, const wchar_t * service, std::error_code & err)
	{
		addrinfo_type hints;

		::ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		int res;
		addrinfo_type * ptr;
		do res = ::GetAddrInfoW(host, service, &hints, &ptr); while (res == WSATRY_AGAIN);
		if (res == 0)
		{
			err.clear();
			return addrinfo_ptr(ptr);
		}
		else
		{
			err.assign(res, std::system_category());
			return nullptr;
		}
	}

	addrinfo_ptr getaddrinfo(const wchar_t * host, const wchar_t * service)
	{
		addrinfo_type hints;

		::ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		int res;
		addrinfo_type * ptr;
		do res = ::GetAddrInfoW(host, service, &hints, &ptr); while (res == WSATRY_AGAIN);
		if (res == 0)
			return addrinfo_ptr(ptr);
		else
			throw_socket_error(res, "GetAddrInfoW failed");
	}

	addrinfo_ptr getaddrinfo(const char * host, const char * service, std::error_code & err)
	{
		std::codecvt_utf8<wchar_t> cvt;

		std::wstring whoststr, wservicestr;

		const wchar_t * whost = nullptr;
		const wchar_t * wservice = nullptr;

		if (host)
		{
			auto in = boost::make_iterator_range_n(host, std::strlen(host));
			ext::codecvt_convert::from_bytes(cvt, in, whoststr);
			whost = whoststr.c_str();
		}

		if (service)
		{
			auto in = boost::make_iterator_range_n(service, std::strlen(service));
			ext::codecvt_convert::from_bytes(cvt, in, wservicestr);
			wservice = wservicestr.c_str();
		}

		return getaddrinfo(whost, wservicestr, err);
	}

	addrinfo_ptr getaddrinfo(const char * host, const char * service)
	{
		std::codecvt_utf8<wchar_t> cvt;

		std::wstring whoststr, wservicestr;

		const wchar_t * whost = nullptr;
		const wchar_t * wservice = nullptr;

		if (host)
		{
			auto in = boost::make_iterator_range_n(host, std::strlen(host));
			ext::codecvt_convert::from_bytes(cvt, in, whoststr);
			whost = whoststr.c_str();
		}

		if (service)
		{
			auto in = boost::make_iterator_range_n(service, std::strlen(service));
			ext::codecvt_convert::from_bytes(cvt, in, wservicestr);
			wservice = wservicestr.c_str();
		}

		return getaddrinfo(whost, wservicestr);
	}

} // namespace ext
