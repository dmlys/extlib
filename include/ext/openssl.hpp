#pragma once
#ifdef EXT_ENABLE_OPENSSL
#include <system_error>
#include <boost/config.hpp>

/// forward some openssl types
struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;

typedef struct ssl_st        SSL;
typedef struct ssl_ctx_st    SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

/// Simple openssl utilities, mostly for socket_stream implementations of ssl functionality.
/// error category, startup/clean up routines, may be something other
namespace ext {
namespace openssl
{
	/// error category for openssl errors from ERR_*
	const std::error_category & openssl_err_category() BOOST_NOEXCEPT;
	/// error category for openssl errors from SSL_*
	const std::error_category & openssl_ssl_category() BOOST_NOEXCEPT;

	/// создает error_code по заданному sslcode полученному с помощью SSL_get_error(..., ret)
	/// если sslcode == SSL_ERROR_SYSCALL, проверяет ERR_get_error() / system error,
	/// если валидны - выставляет их и openssl_err_category,
	/// иначе выставляет openssl_ssl_category
	std::error_code openssl_geterror(int sslcode) BOOST_NOEXCEPT;

	/// per process initialization
	void openssl_init();
	/// per process cleanup
	void openssl_cleanup();
}}

namespace ext
{
	// because those have openssl in theirs names - bring them to ext namespace
	using openssl::openssl_err_category;
	using openssl::openssl_ssl_category;
	using openssl::openssl_geterror;

	using openssl::openssl_init;
	using openssl::openssl_cleanup;
}

#endif // EXT_ENABLE_OPENSSL