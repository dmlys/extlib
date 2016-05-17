// author: Dmitry Lysachenko
// date: Tuesday 17 May 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt
//          

#include <cstdint>
#include <boost/predef.h>

/// Depending on platform some features of bsd sockets can be supported or not,
/// Signatures of functions can be different and so on.
/// this file should forward declare:
/// * addrinfo
/// * sockaddr
/// * socklen_t
/// * sockoptlen_t - type that setsockopt accepts as len parameter, 
///                  normally it should same as socklen_t but on some platforms can be different
///                  
/// define EXT_BSDSOCK_USE_SOTIMEOUT 1/0 - USE SO_RCVTIMEO/SO_SNDTIMEO or select + send/recv pair
/// 

#if BOOST_OS_LINUX

	// linux supports SO_RCVTIMEO, SO_SNDTIMEO
	#define EXT_BSDSOCK_USE_SOTIMEOUT 1
	
	struct addrinfo;
	struct sockaddr;
	typedef unsigned int socklen_t;
	typedef socklen_t    sockoptlen_t;

#elif BOOST_OS_HPUX

	// hp-ux have 2 net libraries, standard libc and libxnet
	// both do not support SO_RCVTIMEO, SO_SNDTIMEO
	// xnet setsockopt returns ENOPROTOOPT, libc setsockopt returns success, but it's noop
	#define EXT_BSDSOCK_USE_SOTIMEOUT 0

	struct addrinfo;
	struct sockaddr;
	typedef std::size_t  socklen_t;

	// if defined _XOPEN_SOURCE - it's libxnet, you probably also should link with -lxnet
	#if defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE >= 500 || defined(_XOPEN_SOURCE_EXTENDED))
	typedef socklen_t  sockoptlen_t;
	#else
	typedef int        sockoptlen_t;
	#endif

#else
	// assume on others that  SO_RCVTIMEO/SO_SNDTIMEO are not supported
	#define EXT_BSDSOCK_USE_SOTIMEOUT 0

	struct addrinfo;
	struct sockaddr;
	typedef unsigned int socklen_t;
	typedef socklen_t    sockoptlen_t;

#endif


#ifdef EXT_ENABLE_OPENSSL
/// forward some openssl types
struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;

typedef struct ssl_st        SSL;
typedef struct ssl_ctx_st    SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
#endif // EXT_ENABLE_OPENSSL
