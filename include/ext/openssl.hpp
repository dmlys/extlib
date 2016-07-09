#pragma once
#ifdef EXT_ENABLE_OPENSSL
#include <system_error>

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
	// those can be used as err == ext::openssl_error::zero_return.
	// also we do not include openssl/ssl.h, those definition are asserted in ext/openssl.cpp
	enum class ssl_error
	{
		none             = 0, // SSL_ERROR_NONE
		ssl              = 1, // SSL_ERROR_SSL
		want_read        = 2, // SSL_ERROR_WANT_READ
		want_write       = 3, // SSL_ERROR_WANT_WRITE
		want_X509_lookup = 4, // SSL_ERROR_WANT_X509_LOOKUP
		syscall          = 5, // SSL_ERROR_SYSCALL
		zero_return      = 6, // SSL_ERROR_ZERO_RETURN
		want_connect     = 7, // SSL_ERROR_WANT_CONNECT
		want_accept      = 8, // SSL_ERROR_WANT_ACCEPT
	};

	/// error category for openssl errors from ERR_*
	const std::error_category & openssl_err_category() noexcept;
	/// error category for openssl errors from SSL_*
	const std::error_category & openssl_ssl_category() noexcept;

	/// интеграция с system_error
	inline std::error_code make_error_code(ssl_error val) noexcept           { return {static_cast<int>(val), openssl_ssl_category()}; }
	inline std::error_condition make_error_condition(ssl_error val) noexcept { return {static_cast<int>(val), openssl_ssl_category()}; }

	/// создает error_code по заданному sslcode полученному с помощью SSL_get_error(..., ret)
	/// если sslcode == SSL_ERROR_SYSCALL, проверяет ERR_get_error() / system error,
	/// если валидны - выставляет их и openssl_err_category,
	/// иначе выставляет openssl_ssl_category
	std::error_code openssl_geterror(int sslcode) noexcept;

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
	
	typedef openssl::ssl_error  openssl_error;
}

namespace std
{
	template <>
	struct is_error_code_enum<ext::openssl::ssl_error> 
		: std::true_type { };
}

#endif // EXT_ENABLE_OPENSSL