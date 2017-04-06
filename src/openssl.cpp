#ifdef EXT_ENABLE_OPENSSL
#include <ext/openssl.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <boost/predef.h> // for BOOST_OS_WINDOWS
#include <boost/static_assert.hpp>

#if BOOST_OS_WINDOWS
#include <windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "crypt32.lib")
#endif // _MSC_VER

#endif // BOOST_OS_WINDOWS

namespace ext {
namespace openssl
{
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::none)              == SSL_ERROR_NONE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::ssl)               == SSL_ERROR_SSL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_read)         == SSL_ERROR_WANT_READ);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_write)        == SSL_ERROR_WANT_WRITE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_X509_lookup)  == SSL_ERROR_WANT_X509_LOOKUP);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::syscall)           == SSL_ERROR_SYSCALL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::zero_return)       == SSL_ERROR_ZERO_RETURN);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_connect)      == SSL_ERROR_WANT_CONNECT);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_error::want_accept)       == SSL_ERROR_WANT_ACCEPT);


	/************************************************************************/
	/*                      error_category                                  */
	/************************************************************************/
	struct openssl_err_category_impl : std::error_category
	{
		const char * name() const noexcept override;
		std::string message(int code) const override;
	};

	const char * openssl_err_category_impl::name() const noexcept
	{
		return "openssl_err";
	}

	std::string openssl_err_category_impl::message(int code) const
	{
		// https://www.openssl.org/docs/manmaster/crypto/ERR_error_string.html
		// openssl says that buffer should be at least 120 bytes long for ERR_error_string.
		const std::size_t buflen = 256;
		char errbuf[buflen];
		::ERR_error_string_n(code, errbuf, buflen);
		// errbuf will be null terminated
		return errbuf;
	}

	struct openssl_ssl_category_impl : std::error_category
	{
		const char * name() const noexcept override;
		std::string message(int code) const override;
	};

	const char * openssl_ssl_category_impl::name() const noexcept
	{
		return "openssl_ssl";
	}

	std::string openssl_ssl_category_impl::message(int code) const
	{
		switch (code)
		{
			case SSL_ERROR_NONE:             return "SSL_ERROR_NONE";
			case SSL_ERROR_SSL:              return "SSL_ERROR_SSL";
			case SSL_ERROR_WANT_READ:        return "SSL_ERROR_WANT_READ";
			case SSL_ERROR_WANT_WRITE:       return "SSL_ERROR_WANT_WRITE";
			case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
			case SSL_ERROR_SYSCALL:          return "SSL_ERROR_SYSCALL";
			
			case SSL_ERROR_ZERO_RETURN:      return "SSL_ERROR_ZERO_RETURN";
			case SSL_ERROR_WANT_CONNECT:     return "SSL_ERROR_WANT_CONNECT";
			case SSL_ERROR_WANT_ACCEPT:      return "SSL_ERROR_WANT_ACCEPT";
			
			default: return "SSL_ERROR_UNKNOWN";
		}
	}
	
	openssl_err_category_impl openssl_err_category_instance;
	openssl_ssl_category_impl openssl_ssl_category_instance;

	const std::error_category & openssl_err_category() noexcept
	{
		return openssl_err_category_instance;
	}

	const std::error_category & openssl_ssl_category() noexcept
	{
		return openssl_ssl_category_instance;
	}

	std::error_code openssl_geterror(int sslcode) noexcept
	{
		if (sslcode == SSL_ERROR_SSL || sslcode == SSL_ERROR_SYSCALL)
		{
			int err = ERR_get_error();
			if (err) return {err, ext::openssl_err_category()};
			
#if BOOST_OS_WINDOWS
			err = ::WSAGetLastError();
#else
			err = errno;
#endif
			if (err) return {err, std::system_category()};
		}
		
		return {sslcode, openssl_ssl_category()};
	}

	/************************************************************************/
	/*                  init/cleanup                                        */
	/************************************************************************/
	void openssl_init()
	{
		SSL_load_error_strings();
		SSL_library_init();
	}

	void openssl_cleanup()
	{
		// https://mta.openssl.org/pipermail/openssl-users/2015-January/000326.html
		CRYPTO_cleanup_all_ex_data();
		ERR_free_strings();
		EVP_cleanup();
	}
}}

#endif // #ifdef EXT_ENABLE_OPENSSL
