#pragma once
#include <ext/iostreams/socket_base.hpp>

/// для windows у нас есть реализация на winsock2
#if BOOST_OS_WINDOWS
#include <ext/iostreams/winsock2_streambuf.hpp>

#ifndef EXT_WINSOCK2_SOCKET_STREAM
#define EXT_WINSOCK2_SOCKET_STREAM
#endif

namespace ext
{
	using socket_streambuf = winsock2_streambuf;
}


/// для остальных случаев откатываемся на bsd реализацию
#else  // BOOST_OS_WINDOWS
#include <ext/iostreams/bsdsock_streambuf.hpp>

#ifndef EXT_BSDSOCK_SOCKET_STREAM
#define EXT_BSDSOCK_SOCKET_STREAM
#endif

namespace ext
{
	using socket_streambuf = bsdsock_streambuf;
}

#endif // BOOST_OS_WINDOWS
