#pragma once
#include <boost/predef.h>

/// для windows у нас есть реализация на winsock2
#if BOOST_OS_WINDOWS
#include <ext/iostreams/winsock2_stream.hpp>

#ifndef EXT_WINSOCK2_SOCKET_STREAM
#define EXT_WINSOCK2_SOCKET_STREAM
#endif

namespace ext
{
	typedef ext::winsock2_streambuf socket_streambuf;
	typedef ext::winsock2_stream    socket_stream;

	/// инициализация библиотек для работы с socket_stream
	inline void socket_stream_init() { ext::winsock2_stream_init(); }
}


/// для остальных случаев откатываемся на bsd реализацию
#else  // BOOST_OS_WINDOWS
#include <ext/iostreams/bsdsock_stream.hpp>

#ifndef EXT_BSDSOCK_SOCKET_STREAM
#define EXT_BSDSOCK_SOCKET_STREAM
#endif

namespace ext
{
	typedef ext::bsdsock_streambuf   socket_streambuf;
	typedef ext::bsdsock_stream      socket_stream;

	/// инициализация библиотек для работы с socket_stream
	inline void socket_stream_init() { bsdsock_stream_init(); }
}

#endif // BOOST_OS_WINDOWS
