#ifdef EXT_ENABLE_OPENSSL
#include <stdio.h>
#include <sys/stat.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/dh.h>
#include <openssl/pkcs12.h>
#include <openssl/crypto.h> // for OPENSSL_gmtime_adj

#include <ext/codecvt_conv/generic_conv.hpp>
#include <ext/codecvt_conv/wchar_cvt.hpp>

#include <boost/predef.h> // for BOOST_OS_WINDOWS
#include <boost/static_assert.hpp>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <ext/openssl.hpp>
#include <ext/time.hpp>
#include <ext/errors.hpp>

#include <cstring> // for std::memset

#if BOOST_OS_WINDOWS
#include <winsock2.h> // winsock2.h should be included before windows.h
#include <windows.h>
//#include <wincrypt.h>

// windows defines macro X509_NAME somewhere in wincrypt, anyways including windows.h bring it into scope
// openssl internally undefs it, we will do the smae
#undef X509
#undef X509_NAME
#undef X509_EXTENSION

#ifdef _MSC_VER
#pragma comment(lib, "crypt32.lib")
#endif // _MSC_VER

#ifndef alloca
#define alloca _alloca
#endif

#ifndef S_ISREG
#define _S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))
#define S_ISREG(mode) _S_ISTYPE((mode), _S_IFREG)
#define S_ISDIR(mode) _S_ISTYPE((mode), _S_IFDIR)
#endif // S_ISREG

#endif // BOOST_OS_WINDOWS


static const char * const ASN1_TIME_GENERALIZED_FMT = "%Y%m%d%H%M%SZ";


int  intrusive_ptr_add_ref(::BIO * ptr)  { return ::BIO_up_ref(ptr); }
void intrusive_ptr_release(::BIO * ptr)  { return ::BIO_free_all(ptr);   }

int  intrusive_ptr_add_ref(::X509 * ptr) { return ::X509_up_ref(ptr); }
void intrusive_ptr_release(::X509 * ptr) { return ::X509_free(ptr);   }

int  intrusive_ptr_add_ref(::RSA * ptr)  { return ::RSA_up_ref(ptr);  }
void intrusive_ptr_release(::RSA * ptr)  { return ::RSA_free(ptr);    }

int  intrusive_ptr_add_ref(::EVP_PKEY * ptr) { return ::EVP_PKEY_up_ref(ptr); }
void intrusive_ptr_release(::EVP_PKEY * ptr) { return ::EVP_PKEY_free(ptr);   }

int  intrusive_ptr_add_ref(::SSL * ptr) { return ::SSL_up_ref(ptr); }
void intrusive_ptr_release(::SSL * ptr) { return ::SSL_free(ptr);   }

int  intrusive_ptr_add_ref(::SSL_CTX * ptr) { return ::SSL_CTX_up_ref(ptr); }
void intrusive_ptr_release(::SSL_CTX * ptr) { return ::SSL_CTX_free(ptr);   }


#if OPENSSL_VERSION_NUMBER < 0x10101000L
static int ASN1_TIME_to_tm(const ::ASN1_TIME * time, std::tm * tm);
#endif

// With OpenSLL v3 some functions now accept some arguments via const pointers.
// Logically they should be accepting those arguments via const pointers from the begining.
// To support both v1 and v3 versions - accept const pointers and unconst them for v1.

// MNNFFPPS: major minor fix patch status
#if OPENSSL_VERSION_NUMBER >= 0x30000000
// for v3, pointers should be already const - do nothing
#define v1_unconst(arg) arg
#else
// for v1, pointers should be unconst
template <class Type>
static inline Type * v1_unconst(const Type * arg) { return const_cast<Type *>(arg); }
#endif

namespace ext::openssl
{
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::none)              == SSL_ERROR_NONE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::ssl)               == SSL_ERROR_SSL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::want_read)         == SSL_ERROR_WANT_READ);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::want_write)        == SSL_ERROR_WANT_WRITE);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::want_X509_lookup)  == SSL_ERROR_WANT_X509_LOOKUP);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::syscall)           == SSL_ERROR_SYSCALL);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::zero_return)       == SSL_ERROR_ZERO_RETURN);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::want_connect)      == SSL_ERROR_WANT_CONNECT);
	BOOST_STATIC_ASSERT(static_cast<int>(ssl_errc::want_accept)       == SSL_ERROR_WANT_ACCEPT);

	const unsigned long rsa_f4 = RSA_F4;

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
		// https://www.openssl.org/docs/man1.1.1/man3/ERR_error_string.html
		// openssl says that buffer should be at least 256 bytes long for ERR_error_string.
		constexpr std::size_t buflen = 512;
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

	std::error_code openssl_geterror(int sslcode, error_retrieve rtype) noexcept
	{
		if (sslcode == SSL_ERROR_SSL || sslcode == SSL_ERROR_SYSCALL)
		{
			int err = rtype == error_retrieve::get ? ::ERR_get_error() : ::ERR_peek_error();
			if (err) return {err, openssl_err_category()};
			
#if BOOST_OS_WINDOWS
			err = ::WSAGetLastError();
#else
			err = errno;
#endif
			if (err) return {err, std::system_category()};
		}
		
		return {sslcode, openssl_ssl_category()};
	}

	void openssl_clear_errors() noexcept
	{
		::ERR_clear_error();
	}

	std::error_code last_error(error_retrieve rtype) noexcept
	{
		int err = rtype == error_retrieve::get ? ::ERR_get_error() : ::ERR_peek_error();
		return std::error_code(err, openssl_err_category());
	}

	EXT_NORETURN void throw_last_error_str(error_retrieve rtype, const std::string & errmsg)
	{
		int err = ::ERR_peek_error();
		std::string error_queue;
		if (rtype == error_retrieve::get)
			error_queue = print_error_queue();
		
		throw openssl_error(std::error_code(err, openssl_err_category()), errmsg, std::move(error_queue));
	}

	static int print_error_queue_string_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pstr = static_cast<std::string *>(ptr);
		pstr->append(data, len);
		return 1;
	}

	static int print_error_queue_streambuf_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pos = static_cast<std::streambuf *>(ptr);
		return pos->sputn(data, len) == static_cast<std::streamsize>(len);
	}

	static int print_error_queue_ostream_cb(const char * data, std::size_t len, void * ptr)
	{
		auto * pos = static_cast<std::ostream *>(ptr);
		pos->write(data, len);
		return static_cast<bool>(*pos);
	}

	void print_error_queue(std::string & str)
	{
		::ERR_print_errors_cb(print_error_queue_string_cb, &str);
	}

	void print_error_queue(std::ostream & os)
	{
		::ERR_print_errors_cb(print_error_queue_ostream_cb, &os);
	}

	void print_error_queue(std::streambuf & os)
	{
		::ERR_print_errors_cb(print_error_queue_streambuf_cb, &os);
	}

	std::string print_error_queue()
	{
		std::string str;
		print_error_queue(str);
		return str;
	}

	/************************************************************************/
	/*                  init/cleanup                                        */
	/************************************************************************/
	void crypto_init()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		::ERR_load_crypto_strings();
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
		//::OPENSSL_init_crypto(0, nullptr);
#endif
	}
	
	void ssl_init()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		::SSL_load_error_strings();
		::SSL_library_init();
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
		//::OPENSSL_init_ssl(0, nullptr);
#endif

	}

	void lib_cleanup()
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		// https://mta.openssl.org/pipermail/openssl-users/2015-January/000326.html
		::CRYPTO_cleanup_all_ex_data();
		::ERR_free_strings();
		::EVP_cleanup();
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
		
#endif
	}

	/************************************************************************/
	/*                  smart ptr stuff                                     */
	/************************************************************************/
	void ssl_deleter::operator()(::SSL * ssl) const noexcept
	{
		::SSL_free(ssl);
	}

	void ssl_ctx_deleter::operator()(::SSL_CTX * sslctx) const noexcept
	{
		::SSL_CTX_free(sslctx);
	}

	void bignum_deleter::operator()(::BIGNUM * bn) const noexcept
	{
		::BN_free(bn);
	}
	
	void bio_deleter::operator()(::BIO * bio) const noexcept
	{
		::BIO_vfree(bio);
	}

	void x509_deleter::operator()(::X509 * cert) const noexcept
	{
		::X509_free(cert);
	}

	void evp_pkey_ctx_deleter::operator()(::EVP_PKEY_CTX * ctx) const noexcept
	{
		::EVP_PKEY_CTX_free(ctx);
	}
	
	void stackof_x509_deleter::operator()(STACK_OF(X509) * x590s) const noexcept
	{
		// https://wiki.nikhef.nl/grid/How_to_handle_OpenSSL_and_not_get_hurt_and_what_does_that_library_call_really_do%3F#Proper_memory_liberation_of_a_STACK_OF_.28X509.29_.2A
		// sk_X509_pop_free should be used instead of sk_X509_free, or memory leaks will occur or certificate chains longer than 1
		::sk_X509_pop_free(x590s, ::X509_free);
	}
	
	void stack_st_x509_extension_deleter::operator()(stack_st_X509_EXTENSION * extensions) const noexcept
	{
		::sk_X509_EXTENSION_pop_free(extensions, ::X509_EXTENSION_free);
	}

	void rsa_deleter::operator()(RSA * rsa) const noexcept
	{
		::RSA_free(rsa);
	}

	void evp_pkey_deleter::operator()(::EVP_PKEY * pkey) const noexcept
	{
		::EVP_PKEY_free(pkey);
	}

	void pkcs12_deleter::operator()(::PKCS12 * pkcs12) const noexcept
	{
		::PKCS12_free(pkcs12);
	}
	
	void x509_extension_deleter::operator()(::X509_EXTENSION *extension) const noexcept
	{
		::X509_EXTENSION_free(extension);
	}

	/************************************************************************/
	/*                  print helpers                                       */
	/************************************************************************/
	std::string x509_name_string(const ::X509_NAME * name, int flags)
	{
		ext::openssl::bio_uptr mem_bio(::BIO_new(::BIO_s_mem()));
		if (not mem_bio) throw_last_error("ext::openssl::x509_name_string: ::BIO_new failed");
		
		// XN_FLAG_DN_REV - for XML DSIG, names subject and issuers should be in reversed order
		if (-1 == ::X509_NAME_print_ex(mem_bio.get(), name, 0, flags))
			ext::openssl::throw_last_error("ext::openssl::x509_name_string: ::X509_NAME_print_ex failed");

		char * data;
		int len = ::BIO_get_mem_data(mem_bio.get(), &data);
		return std::string(data, len);
	}
	
	std::string x509_name_string(const ::X509_NAME * name)
	{
		constexpr int flags = (XN_FLAG_RFC2253 | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB;
		return x509_name_string(name, flags);
	}
	
	std::string x509_name_reversed_string(const ::X509_NAME * name)
	{
		// XN_FLAG_DN_REV - names subject and issuers should be in reversed order
		constexpr int flags = (XN_FLAG_RFC2253 | XN_FLAG_DN_REV | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB;
		return x509_name_string(name, flags);
	}
	
	std::string bignum_string(const ::BIGNUM * num)
	{
		auto * str = ::BN_bn2dec(num);
		std::string result = str;
		::OPENSSL_free(str);
		return result;
	}
	
	std::string asn1_integer_string(const ::ASN1_INTEGER * integer)
	{
		auto * bn = ::ASN1_INTEGER_to_BN(integer, nullptr);
		auto result = bignum_string(bn);
		::BN_free(bn);

		return result;
	}
	
	static int utc_offset()
	{
		// Calculate difference between local time and UTC
		// by breaking current UNIX time stamp into tm struct,
		// and converting back as it was local time.
		//
		// That way both timezone and daylight saving will be taken into account.
		//
		// for + timezones result offset will be positive
		// for - timezones result offset will be negative
	
		std::time_t t1, t2;
		std::tm tm;
	
		t1 = time(nullptr);
	
	#ifdef _WIN32
		gmtime_s(&tm, &t1); // to UTC time
	#else
		gmtime_r(&t1, &tm); // to UTC time
	#endif
	
		t2 = mktime(&tm);   // from local time
	
		// offset should not be bigger than a day
		assert(-86400 < t1 - t2 && t1 - t2 < 86400);
	
		return t1 - t2;
	}
	
	std::string asn1_time_string(const ::ASN1_TIME * time, timezone tz, const char * fmt, int max_size)
	{
		if (time == nullptr)
			return "(null)";
		
		// RFC 5280 among other things states that date/time fields(including not after, not before)
		// should be encoded as ASN1 UTCTime or GeneralizedTime.
		// Those are actually strings in format YYMMDDHHMMSSZ / YYYYMMDDHHMMSSZ.
		// So we already have broken down time, ASN1_TIME_to_tm parses string into struct tm.
		//
		// That time is UTC. If we want local time - we could convert to time_t via timegm and back via localtime.
		// timegm is not very portable: some platforms have it, others don't.
		//
		// On windows localtime, gmtime and others actually can't work with dates bigger than _MAX__TIME64_T
		// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-s-localtime32-s-localtime64-s?view=msvc-170
		// _MAX__TIME64_T is not public constant and defined in internal header, in practice it is limited by end of 3rd millennia.
		// For dates like 3002-01-12 - localtime_r returns EINVAL and sets struct tm to 1970.
		//
		//   It happens that some people do make certificates with "not after" in 3020-12-31 year...
		//
		// OpenSSL have platform agnostic function OPENSSL_gmtime_adj - it adds to struct tm offset number of days and seconds.
		// Internally it's implemented by converting to Julian day(Uses Fliegel & Van Flandern algorithm).
		// It works with any dates(probably)
		//
		// So for UTC we already have broken time, and for localtime add utc offset with OPENSSL_gmtime_adj
		// UTC offset can be calculated by taking current time(time), breaking it into tm as UTC(gmtime), converting it back as if localtime(mktime).
		
		std::tm tm;
		if (::ASN1_TIME_to_tm(time, &tm) != 1)
			throw_last_error("ext::openssl::asn1_time_string: ASN1_TIME_to_tm failed");
		
		if (tz == timezone::localtime)
		{
		#if not BOOST_OS_WINDOWS
			auto t = ext::mkgmtime(&tm);
			ext::localtime(&t, &tm);
		#else // BOOST_OS_WINDOWS
			constexpr int seconds_per_day = 60 * 60 * 24;
			int seconds, days;
	
			seconds = utc_offset();
			days = seconds / seconds_per_day;    // in practice days should be always zero
			seconds = seconds % seconds_per_day;
	
			OPENSSL_gmtime_adj(&tm, days, seconds);
		#endif
		}
				
	#if BOOST_OS_UNIX or BOOST_OS_WINDOWS
		char * buffer = static_cast<char *>(alloca(max_size));
		
		errno = 0;
		auto written = std::strftime(buffer, max_size, fmt, &tm);
		if (written == 0 and errno)
			ext::throw_last_errno("ext::openssl::asn1_time_string: strftime failed");
		
		return std::string(buffer, written);
	#else
		std::string result;
		result.resize(max_size);
		
		errno = 0;
		auto written = std::strftime(result.data(), max_size, fmt, &tm);
		if (written == 0 and errno)
			ext::throw_last_errno("ext::openssl::asn1_time_string: strftime failed");
		
		result.resize(written);
		return result;
	#endif
	}


	time_t asn1_time_timet(const ::ASN1_TIME * time)
	{
		// TODO: handle special case 99991231235959Z
		auto tm = asn1_time_tm(time);
		return ext::mkgmtime(&tm);
	}
	
	std::tm asn1_time_tm(const ::ASN1_TIME * time)
	{
		std::tm tm;
		if (1 != ::ASN1_TIME_to_tm(time, &tm))
			ext::openssl::throw_last_error("ext::openssl::asn1_time_tm: ::ASN1_TIME_to_tm failed");
		
		return tm;
	}
	
	ASN1_TIME * asn1_time_tm(::ASN1_TIME * time, const std::tm * utc_tm)
	{
		assert(utc_tm);
		
		constexpr unsigned N = 64;
		char buffer[N];
		
		if (not time)
			time = ASN1_TIME_new();
		
		std::strftime(buffer, N, ASN1_TIME_GENERALIZED_FMT, utc_tm);
		if (ASN1_TIME_set_string_X509(time, buffer) != 1)
			throw_last_error("ext::openssl::asn1_time_tm: ::ASN1_TIME_set_string_X509 failed");
		
		return time;
	}
	
	std::string asn1_time_print(const ::ASN1_TIME * time)
	{
		ext::openssl::bio_uptr mem_bio(::BIO_new(::BIO_s_mem()));
		if (not mem_bio) throw_last_error("ext::openssl::asn1_time_print: ::BIO_new failed");

		if (1 != ::ASN1_TIME_print(mem_bio.get(), time))
			ext::openssl::throw_last_error("ext::openssl::asn1_time_print: ::ASN1_TIME_print failed");
		
		char * data;
		int len = ::BIO_get_mem_data(mem_bio.get(), &data);
		return std::string(data, len);
	}
	
	::ASN1_TIME * asn1_time_set(::ASN1_TIME * time, time_t tt)
	{
		time = ::ASN1_TIME_set(time, tt);
		return time;
	}
	
	
	/************************************************************************/
	/*                  utility stuff                                       */
	/************************************************************************/
	static bool is_pem(sformat form, const char * data, std::size_t len)
	{
		switch (form)
		{
			case sformat::PEM: return true;
			case sformat::DER: return false;
			case sformat::auto_:
			{
				std::string_view str(data, len);
				std::string_view::size_type cur = 0;
				while (cur < str.size())
				{
					auto line_end = str.find('\n', cur);
					auto line = str.substr(cur, line_end - cur);
					auto nspace_pos = line.find_first_not_of(" \t\r\0");
					if (nspace_pos == line.npos)
					{
						cur = line_end + 1;
						continue;
					}
					
					return line.compare(nspace_pos, 5, "-----") == 0;
				}
				
				return false;
			}
			
			default:
				EXT_UNREACHABLE();
		}
	}
	
	static bool is_pem(sformat form, std::FILE * fp)
	{
		switch (form)
		{
			case sformat::PEM: return true;
			case sformat::DER: return false;
			case sformat::auto_:
			{
				constexpr unsigned N = 512;
				char buffer[N + 4];
				
				struct stat stat;
				if (fstat(fileno(fp), &stat) != 0)
					ext::throw_last_errno("ext::openssl::is_pem: fstat failure");
				
				if (not S_ISREG(stat.st_mode))
					return true;
				
				while (std::fgets(buffer, N + 1, fp))
				{
					auto isspace = [](char c) { return c == ' ' or c == '\t' or c == '\r' or c == '\0'; };
					auto it = std::find_if_not(buffer, buffer + N, isspace);
					if (it == buffer + N or *it == '\n')
						continue;
					
					std::rewind(fp);
					if (errno)
						ext::throw_last_errno("ext::openssl::is_pem: std::rewind failure");
					
					return std::strncmp(it, "-----", 5) == 0;
				}
				
				std::rewind(fp);
				if (errno)
					ext::throw_last_errno("ext::openssl::is_pem: std::rewind failure");
				
				return false;
			}
			
			default:
				EXT_UNREACHABLE();
		}
	}
	
	struct FILE_closer
	{
		void operator()(FILE * f) const noexcept { std::fclose(f); }
	};
	
	static int empty_password_callback(char * buff, int bufsize, int rwflag, void * userdata)
	{
		*buff = 0;
		return 0;
	}
	
	static int (* const PASSWORD_CALLBACK)(char * buff, int bufsize, int rwflag, void * userdata) = &empty_password_callback;

	x509_iptr load_certificate(const char * data, std::size_t len, sformat form)
	{
		bio_uptr bio_uptr;

		auto * bio = ::BIO_new_mem_buf(data, static_cast<int>(len));
		bio_uptr.reset(bio);
		if (not bio)
			throw_last_error("ext::openssl::load_certificate: ::BIO_new_mem_buf failed");
		
		::X509 * cert;
		if (is_pem(form, data, len))
		{
			cert = ::PEM_read_bio_X509(bio, nullptr, PASSWORD_CALLBACK, nullptr);
			if (not cert)
				throw_last_error("ext::openssl::load_certificate: ::PEM_read_bio_X509 failed");
		}
		else
		{
			cert = ::d2i_X509_bio(bio, nullptr);
			if (not cert)
				throw_last_error("ext::openssl::load_certificate: ::d2i_X509_bio failed");
		}
		
		return x509_iptr(cert, ext::noaddref);
	}

	evp_pkey_iptr load_private_key(const char * data, std::size_t len, sformat form)
	{
		bio_uptr bio_uptr;

		auto * bio = ::BIO_new_mem_buf(data, static_cast<int>(len));
		bio_uptr.reset(bio);
		if (not bio)
			throw_last_error("ext::openssl::load_private_key: ::BIO_new_mem_buf failed");
		
		::EVP_PKEY * pkey;
		if (is_pem(form, data, len))
		{
			pkey = ::PEM_read_bio_PrivateKey(bio, nullptr, PASSWORD_CALLBACK, nullptr);
			if (not pkey)
				throw_last_error("ext::openssl::load_private_key: ::PEM_read_bio_PrivateKey failed");
		}
		else
		{
			pkey = ::d2i_PrivateKey_bio(bio, nullptr);
			if (not pkey)
				throw_last_error("ext::openssl::load_private_key: ::d2i_PrivateKey_bio failed");
		}
		
		return evp_pkey_iptr(pkey, ext::noaddref);
	}
	
	std::string write_certificate(const ::X509 * cert, sformat form)
	{
		assert(cert);
		
		auto * mem_bio = ::BIO_new(::BIO_s_mem());
		if (not mem_bio) throw_last_error("ext::openssl::write_certificate: ::BIO_new failed");
		bio_uptr bio_ptr(mem_bio);
		
		int res;
		switch (form)
		{
			case sformat::auto_:
			case sformat::PEM:
				res = ::PEM_write_bio_X509(mem_bio, v1_unconst(cert));
				if (not res)
					throw_last_error("ext::openssl::write_certificate: ::PEM_write_bio_X509 failed");
				
				break;
				
			case sformat::DER:
				res = ::i2d_X509_bio(mem_bio, v1_unconst(cert));
				if (not res)
					throw_last_error("ext::openssl::write_certificate: ::i2d_X509_bio failed");
				
				break;
			
			default:
				EXT_UNREACHABLE();
		}
				
		char * data;
		int len = ::BIO_get_mem_data(mem_bio, &data);
		
		return std::string(data, len);
	}
	
	std::string write_private_key(const ::EVP_PKEY * key, sformat form)
	{
		assert(key);
		
		auto * mem_bio = ::BIO_new(::BIO_s_mem());
		if (not mem_bio) throw_last_error("ext::openssl::write_pkey: ::BIO_new failed");
		bio_uptr bio_ptr(mem_bio);
		
		int res;
		switch (form)
		{
			case sformat::auto_:
			case sformat::PEM:
				res = ::PEM_write_bio_PrivateKey(mem_bio, v1_unconst(key), nullptr, nullptr, 0, nullptr, nullptr);
				if (not res)
					throw_last_error("ext::openssl::write_pkey: ::PEM_write_bio_X509 failed");
				
				break;
				
			case sformat::DER:
				res = ::i2d_PrivateKey_bio(mem_bio, v1_unconst(key));
				if (not res)
					throw_last_error("ext::openssl::write_pkey: ::i2d_PrivateKey_bio failed");
				
				break;
				
			default:
				EXT_UNREACHABLE();
		}
				
		char * data;
		int len = ::BIO_get_mem_data(mem_bio, &data);
		
		return std::string(data, len);
	}

	x509_iptr load_certificate_from_file(const char * path, sformat form)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::unique_ptr<std::FILE, FILE_closer> fp(::_wfopen(wpath.c_str(), L"rb"));
#else
		std::unique_ptr<std::FILE, FILE_closer> fp(std::fopen(path, "rb"));
#endif
		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_certificate_from_file: std::fopen failed");
		}

		return load_certificate_from_file(fp.get(), form);
	}

	x509_iptr load_certificate_from_file(std::FILE * file, sformat form)
	{
		assert(file);
		::X509 * cert;
		if (is_pem(form, file))
		{
			cert = ::PEM_read_X509(file, nullptr, PASSWORD_CALLBACK, nullptr);
			if (not cert)
				throw_last_error("ext::openssl::load_certificate_from_file: ::PEM_read_X509 failed");
		}
		else
		{
			cert = ::d2i_X509_fp(file, nullptr);
			if (not cert)
				throw_last_error("ext::openssl::load_certificate_from_file: ::d2i_X509_fp failed");
		}
		
		return x509_iptr(cert, ext::noaddref);
	}


	evp_pkey_iptr load_private_key_from_file(const char * path, sformat form)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::unique_ptr<std::FILE, FILE_closer> fp(::_wfopen(wpath.c_str(), L"rb"));
#else
		std::unique_ptr<std::FILE, FILE_closer> fp(std::fopen(path, "rb"));
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_private_key_from_file: std::fopen failed");
		}

		return load_private_key_from_file(fp.get(), form);
	}

	evp_pkey_iptr load_private_key_from_file(std::FILE * file, sformat form)
	{
		assert(file);
		
		//                  traditional or PKCS#8 format
		::EVP_PKEY * pkey;
		if (is_pem(form, file))
		{
			pkey = ::PEM_read_PrivateKey(file, nullptr, PASSWORD_CALLBACK, nullptr);
			if (not pkey)
				throw_last_error("ext::openssl::load_private_key_from_file: ::PEM_read_PrivateKey failed");
		}
		else
		{
			pkey = ::d2i_PrivateKey_fp(file, nullptr);
			if (not pkey)
				throw_last_error("ext::openssl::load_private_key_from_file: ::d2i_PrivateKey_fp failed");
		}
		
		return evp_pkey_iptr(pkey, ext::noaddref);
	}
	
	void write_certificate_to_file(std::FILE * fp, const ::X509 * cert, sformat form)
	{
		assert(fp);
		assert(cert);
		
		int res;
		switch (form)
		{
			case sformat::auto_:
			case sformat::PEM:
				res = ::PEM_write_X509(fp, v1_unconst(cert));
				if (not res)
					throw_last_error("ext::openssl::write_certificate_to_file: ::PEM_write_X509 failed");
				
				break;
				
			case sformat::DER:
				res = ::i2d_X509_fp(fp, v1_unconst(cert));
				if (not res)
					throw_last_error("ext::openssl::write_certificate_to_file: ::i2d_X509_fp failed");
				
				break;
				
			default:
				EXT_UNREACHABLE();
		}
	}
	
	void write_certificate_to_file(const char * path, const ::X509 * cert, sformat form)
	{
		assert(path);
		assert(cert);
		
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::unique_ptr<std::FILE, FILE_closer> fp(::_wfopen(wpath.c_str(), L"wb"));
#else
		std::unique_ptr<std::FILE, FILE_closer> fp(std::fopen(path, "wb"));
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::write_certificate_to_file: std::fopen failed");
		}
		
		return write_certificate_to_file(fp.get(), cert, form);
	}
	
	void write_private_key_to_file(std::FILE * fp, const ::EVP_PKEY * pkey, sformat form)
	{
		assert(fp);
		assert(pkey);
		
		int res;
		switch (form)
		{
			case sformat::auto_:
			case sformat::PEM:
				res = ::PEM_write_PrivateKey(fp, v1_unconst(pkey), nullptr, nullptr, 0, nullptr, nullptr);
				if (not res)
					throw_last_error("ext::openssl::write_pkey_to_file: ::PEM_write_PrivateKey failed");
				
				break;
				
			case sformat::DER:
				res = ::i2d_PrivateKey_fp(fp, v1_unconst(pkey));
				if (not res)
					throw_last_error("ext::openssl::write_pkey_to_file: ::PEM_write_PrivateKey failed");
				
				break;
			
			default:
				EXT_UNREACHABLE();
		}
	}
	
	void write_private_key_to_file(const char * path, const ::EVP_PKEY * pkey, sformat form)
	{
		assert(path);
		assert(pkey);
		
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::unique_ptr<std::FILE, FILE_closer> fp(::_wfopen(wpath.c_str(), L"wb"));
#else
		std::unique_ptr<std::FILE, FILE_closer> fp(std::fopen(path, "wb"));
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::write_pkey_to_file: std::fopen failed");
		}
		
		return write_private_key_to_file(fp.get(), pkey, form);
	}

	pkcs12_uptr load_pkcs12(const char * data, std::size_t len)
	{
		bio_uptr source_bio(::BIO_new_mem_buf(data, static_cast<int>(len)));
		if (not source_bio) throw_last_error("ext::openssl::load_pkcs12: ::BIO_new_mem_buf failed");

		pkcs12_uptr pkcs12(::d2i_PKCS12_bio(source_bio.get(), nullptr));
		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	pkcs12_uptr load_pkcs12_from_file(const char * path)
	{
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::FILE * fp = ::_wfopen(wpath.c_str(), L"rb");
#else
		std::FILE * fp = std::fopen(path, "rb");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::load_pkcs12_from_file: std::fopen failed");
		}

		pkcs12_uptr pkcs12(::d2i_PKCS12_fp(fp, nullptr));
		std::fclose(fp);

		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}

	pkcs12_uptr load_pkcs12_from_file(std::FILE * file)
	{
		assert(file);
		pkcs12_uptr pkcs12(::d2i_PKCS12_fp(file, nullptr));

		if (not pkcs12) throw_last_error("ext::openssl::load_pkcs12_from_file: ::d2i_PKCS12_fp failed");
		return pkcs12;
	}
	
	std::vector<char> write_pkcs12(const ::PKCS12 * pkcs12)
	{
		assert(pkcs12);
		
		auto * mem_bio = ::BIO_new(::BIO_s_mem());
		if (not mem_bio) throw_last_error("ext::openssl::write_pkey: ::BIO_new failed");
		bio_uptr bio_ptr(mem_bio);
		
		int res = ::i2d_PKCS12_bio(mem_bio, v1_unconst(pkcs12));
		if (not res) throw_last_error("ext::openssl::write_pkcs12: ::i2d_PKCS12_bio failed");
		
		char * data;
		int len = ::BIO_get_mem_data(mem_bio, &data);
		
		return std::vector(data, data + len);
	}
	
	/// Writes PKCS12 into given file in DER format.
	/// Passwork protection is set by other functions, this is just a serializtion function.
	/// Throws std::system_error in case of errors
	void write_pkcs12_to_file(std::FILE * fp, const ::PKCS12 * pkcs12)
	{
		assert(fp);
		assert(pkcs12);
		
		int res = ::i2d_PKCS12_fp(fp, v1_unconst(pkcs12));
		if (not res) throw_last_error("ext::openssl::write_pkcs12_to_file: ::i2d_PKCS12_fp failed");
	}
	
	void write_pkcs12_to_file(const char * path, const ::PKCS12 * pkcs12)
	{
		assert(path);
		assert(pkcs12);
		
#if BOOST_OS_WINDOWS
		auto wpath = ext::codecvt_convert::wchar_cvt::to_wchar(path);
		std::FILE * fp = ::_wfopen(wpath.c_str(), L"wb");
#else
		std::FILE * fp = std::fopen(path, "wb");
#endif

		if (fp == nullptr)
		{
			std::error_code errc(errno, std::generic_category());
			throw std::system_error(errc, "ext::openssl::write_pkcs12_to_file: std::fopen failed");
		}
		
		int res = ::i2d_PKCS12_fp(fp, v1_unconst(pkcs12));
		fclose(fp);
		
		if (not res) throw_last_error("ext::openssl::write_pkcs12_to_file: ::i2d_PKCS12_fp failed");
	}
	
	bool pkcs12_password_encrypted(::PKCS12 * p12, std::string_view pass)
	{
		int res = ::PKCS12_verify_mac(p12, pass.data(), pass.length());
		if (res == 1) return true;
		
		int err = ::ERR_peek_error();
		int lib = ERR_GET_LIB(err);
		if (err and lib == ERR_LIB_PKCS12)
			return false;
		
		if (not pass.empty()) return false;
		
		res = ::PKCS12_verify_mac(p12, nullptr, 0);
		return res == 1;
	}
	
	void parse_pkcs12(::PKCS12 * pkcs12, const char * passwd, evp_pkey_iptr & evp_pkey, x509_iptr & x509, stackof_x509_uptr & ca)
	{
		::X509 * raw_cert = nullptr;
		::EVP_PKEY * raw_pkey = nullptr;
		STACK_OF(X509) * raw_ca = nullptr;

		int res = ::PKCS12_parse(pkcs12, passwd, &raw_pkey, &raw_cert, &raw_ca);
		if (res <= 0) throw_last_error("ext::openssl::parse_pkcs12: ::PKCS12_parse failed");

		evp_pkey.reset(raw_pkey, ext::noaddref);
		x509.reset(raw_cert, ext::noaddref);
		ca.reset(raw_ca);
	}

	auto parse_pkcs12(::PKCS12 * pkcs12, const char * passwd) -> std::tuple<evp_pkey_iptr, x509_iptr, stackof_x509_uptr>
	{
		std::tuple<evp_pkey_iptr, x509_iptr, stackof_x509_uptr> result;
		parse_pkcs12(pkcs12, passwd, std::get<0>(result), std::get<1>(result), std::get<2>(result));
		return result;
	}
	
	pkcs12_uptr create_pkcs12(const char * pass, const char * name,
	                          ::EVP_PKEY * pkey, ::X509 * cert, ::stack_st_X509 * ca,
	                          int nid_key, int nid_cert, int iter, int mac_iter, int keytype)
	{
		auto * pkcs12 = ::PKCS12_create(pass, name, pkey, cert, ca, nid_key, nid_cert, iter, mac_iter, keytype);
		if (not pkcs12)
			throw_last_error("ext::openssl::create_pkcs12: ::PKCS12_create failed");
		
		return pkcs12_uptr(pkcs12);
	}


	ssl_ctx_iptr create_sslctx(::X509 * cert, ::EVP_PKEY * pkey, ::stack_st_X509 * ca_chain)
	{
		auto * method = ::SSLv23_server_method();
		return create_sslctx(method, cert, pkey, ca_chain);
	}

	ssl_ctx_iptr create_sslctx(const ::SSL_METHOD * method, ::X509 * cert, ::EVP_PKEY * pkey, ::stack_st_X509 * ca_chain)
	{
		auto * ctx = ::SSL_CTX_new(method);

		ssl_ctx_iptr ssl_ctx_iptr(ctx, ext::noaddref);

		if (::SSL_CTX_use_cert_and_key(ctx, cert, pkey, ca_chain, 1) != 1)
			throw_last_error("ext::openssl::create_sslctx: ::SSL_CTX_use_cert_and_key failed");

		return ssl_ctx_iptr;
	}

	ssl_ctx_iptr create_anonymous_sslctx()
	{
		auto * method = ::SSLv23_server_method();
		return create_anonymous_sslctx(method);
	}

	ssl_ctx_iptr create_anonymous_sslctx(const ::SSL_METHOD * method)
	{
		auto * ctx = ::SSL_CTX_new(method);

		ssl_ctx_iptr ssl_ctx_iptr(ctx, ext::noaddref);
		if (::SSL_CTX_set_cipher_list(ctx, "aNULL:eNULL") != 1)
			throw_last_error("ext::openssl::create_anonymous_sslctx: ::SSL_CTX_set_cipher_list failed");

		::DH * dh = ::DH_get_2048_256();
		::SSL_CTX_set_tmp_dh(ctx, dh);
		::DH_free(dh);

		return ssl_ctx_iptr;
	}
	
	/************************************************************************/
	/*                  not_before/not_after date stuff                     */
	/************************************************************************/
	void get_notbefore(const ::X509 * cert, std::tm * utc_tpoint)
	{
		assert(cert);
		assert(utc_tpoint);
		
		const auto * notbefore = ::X509_get0_notBefore(cert);
		if (ASN1_TIME_to_tm(notbefore, utc_tpoint) != 1)
			throw_last_error("ext::openssl::get_notbefore: ::ASN1_TIME_to_tm failed");
	}
	
	void get_notbefore(const ::X509 * cert, std::time_t * tpoint)
	{
		assert(cert);
		assert(tpoint);
		
		const auto * notbefore = ::X509_get0_notBefore(cert);
		
		std::tm utc;
		if (ASN1_TIME_to_tm(notbefore, &utc) != 1)
			throw_last_error("ext::openssl::get_notbefore: ::ASN1_TIME_to_tm failed");
		
		*tpoint = ext::mkgmtime(&utc);
	}
	
	void get_notbefore(const ::X509 * cert, std::chrono::system_clock::time_point * tpoint)
	{
		std::time_t t;
		get_notbefore(cert, &t);
		
		*tpoint = std::chrono::system_clock::from_time_t(t);
	}
	
	auto get_notbefore(const ::X509 * cert) -> std::chrono::system_clock::time_point
	{
		std::chrono::system_clock::time_point tpoint;
		get_notbefore(cert, &tpoint);
		return tpoint;
	}
	
	
	void get_notafter(const ::X509 * cert, std::tm * utc_tpoint)
	{
		assert(cert);
		assert(utc_tpoint);
		
		const auto * notbefore = ::X509_get0_notAfter(cert);
		if (ASN1_TIME_to_tm(notbefore, utc_tpoint) != 1)
			throw_last_error("ext::openssl::get_notbefore: ::ASN1_TIME_to_tm failed");
	}
	
	void get_notafter(const ::X509 * cert, std::time_t * tpoint)
	{
		assert(cert);
		assert(tpoint);
		
		const auto * notbefore = ::X509_get0_notAfter(cert);
		
		std::tm utc;
		if (ASN1_TIME_to_tm(notbefore, &utc) != 1)
			throw_last_error("ext::openssl::get_notbefore: ::ASN1_TIME_to_tm failed");
		
		*tpoint = ext::mkgmtime(&utc);
	}
	
	void get_notafter(const ::X509 * cert, std::chrono::system_clock::time_point * tpoint)
	{
		std::time_t t;
		get_notafter(cert, &t);
		
		*tpoint = std::chrono::system_clock::from_time_t(t);
	}
	
	auto get_notafter(const ::X509 * cert) -> std::chrono::system_clock::time_point
	{
		std::chrono::system_clock::time_point t;
		get_notafter(cert, &t);
		return t;
	}
	
	
	
	void set_notbefore(::X509 * cert, const std::tm * utc_tpoint)
	{
		assert(cert);
		
		constexpr unsigned N = 64;
		char buffer[N];
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		std::strftime(buffer, N, ASN1_TIME_GENERALIZED_FMT, utc_tpoint);
		if (ASN1_TIME_set_string_X509(notbefore, buffer) != 1)
			throw_last_error("ext::openssl::set_notbefore: ::ASN1_TIME_set_string_X509 failed");
	}
	
	void set_notbefore(::X509 * cert, std::time_t tpoint)
	{
		assert(cert);
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		ASN1_TIME_set(notbefore, tpoint);
	}
	
	void set_notbefore(::X509 * cert, std::chrono::system_clock::time_point tpoint)
	{
		assert(cert);
		
		std::time_t t = std::chrono::system_clock::to_time_t(tpoint);
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		ASN1_TIME_set(notbefore, t);
	}
	
	void set_notafter(::X509 * cert, const std::tm * utc_tpoint)
	{
		assert(cert);
		
		constexpr unsigned N = 64;
		char buffer[N];
		
		auto * notafter = ::X509_getm_notAfter(cert);
		std::strftime(buffer, N, ASN1_TIME_GENERALIZED_FMT, utc_tpoint);
		if (ASN1_TIME_set_string_X509(notafter, buffer) != 1)
			throw_last_error("ext::openssl::set_notafter: ::ASN1_TIME_set_string_X509 failed");
	}
	
	void set_notafter(::X509 * cert, std::time_t tpoint)
	{
		assert(cert);
		
		auto * notbefore = ::X509_getm_notAfter(cert);
		ASN1_TIME_set(notbefore, tpoint);
	}
	
	void set_notafter(::X509 * cert, std::chrono::system_clock::time_point tpoint)
	{
		assert(cert);
		
		std::time_t t = std::chrono::system_clock::to_time_t(tpoint);
		
		auto * notafter = ::X509_getm_notAfter(cert);
		ASN1_TIME_set(notafter, t);
	}
	
	
	
	void set_duration(::X509 * cert, const std::tm * utc_not_before, const std::tm * utc_not_after)
	{
		assert(cert);
		assert(utc_not_before);
		assert(utc_not_after);
				
		constexpr unsigned N = 64;
		char buffer[N];
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		auto * notafter  = ::X509_getm_notAfter(cert);
		
		std::strftime(buffer, N, ASN1_TIME_GENERALIZED_FMT, utc_not_before);
		if (ASN1_TIME_set_string_X509(notbefore, buffer) != 1)
			throw_last_error("ext::openssl::set_duration: ::ASN1_TIME_set_string_X509(not_before) failed");
		
		std::strftime(buffer, N, ASN1_TIME_GENERALIZED_FMT, utc_not_after);
		if (ASN1_TIME_set_string_X509(notafter, buffer) != 1)
			throw_last_error("ext::openssl::set_duration: ::ASN1_TIME_set_string_X509(not_after) failed");
	}
	
	void set_duration(::X509 * cert, std::chrono::system_clock::time_point not_before, std::chrono::system_clock::time_point not_after)
	{
		assert(cert);
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		auto * notafter  = ::X509_getm_notAfter(cert);
		
		::ASN1_TIME_set(notbefore, std::chrono::system_clock::to_time_t(not_before));
		::ASN1_TIME_set(notafter, std::chrono::system_clock::to_time_t(not_after));
	}
	
	void set_duration(::X509 * cert, std::chrono::system_clock::time_point not_before, std::chrono::system_clock::duration duration)
	{
		assert(cert);
		
		auto * notbefore = ::X509_getm_notBefore(cert);
		auto * notafter  = ::X509_getm_notAfter(cert);
		
		::ASN1_TIME_set(notbefore, std::chrono::system_clock::to_time_t(not_before));
		::ASN1_STRING_copy(notafter, notbefore); // notafter <- notbefore
		
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
		::X509_gmtime_adj(notafter, seconds.count());
	}
	
	void set_duration(::X509 * cert, std::chrono::system_clock::duration duration)
	{
		assert(cert);
		
		auto * notbefore = ::X509_get0_notBefore(cert);
		auto * notafter  = ::X509_getm_notAfter(cert);
		::ASN1_STRING_copy(notafter, notbefore); // notafter <- notbefore
		
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
		::X509_gmtime_adj(notafter, seconds.count());
	}
	
	std::vector<unsigned char> cert_sha1fingerprint(const ::X509 * cert)
	{
		assert(cert);
		
		std::vector<unsigned char> result;
		unsigned ressize = 20; // SHA1 produces 160-bit (20-byte) hash value
		result.resize(ressize);
		
		auto sha1_evp = ::EVP_sha1();
		int res = ::X509_digest(cert, sha1_evp, result.data(), &ressize);
		if (not res) throw_last_error("ext::openssl::cert_sha1fingerprint: ::X509_digest failed");
		
		assert(ressize == 20);
		//result.resize(ressize);
		return result;
	}
	
	void x509_name_add_entry_by_txt(::X509_NAME * cert_name, const std::string & name, std::string_view value)
	{
		assert(cert_name);
		
		int res = ::X509_NAME_add_entry_by_txt(cert_name, name.c_str(), MBSTRING_UTF8, reinterpret_cast<const unsigned char *>(value.data()), value.size(), -1, 0);
		if (not res) throw_last_error("ext::openssl::x509_name_add_entry_by_txt: ::X509_NAME_add_entry_by_txt failed");
	}
	
	void x509_name_add_entry_by_txt(::X509_NAME * cert_name, std::string name_str)
	{
		assert(cert_name);
		using token_function = boost::escaped_list_separator<char>;
		using tokenizer_type = boost::tokenizer<token_function>;
		
		tokenizer_type tokenizer(name_str, token_function("\\", ", ", "\""));
		for (auto & tok : tokenizer)
		{
			if (tok.empty()) continue;
			
			auto pos = tok.find('=');
			if (pos == tok.npos) continue;
			
			auto name = tok.substr(0, pos);
			auto value = tok.substr(pos + 1);
			
			boost::trim_if(name, boost::is_any_of(" "));
			boost::trim_if(value, boost::is_any_of(" "));
			
			x509_name_add_entry_by_txt(cert_name, name, value);
		}
	}
	
	x509_extension_uptr create_extension_by_nid(unsigned long nid, std::string value)
	{
		auto cert_ext = ::X509V3_EXT_nconf_nid(nullptr, nullptr, nid, value.c_str());
		if (not cert_ext) throw_last_error("ext::openssl::create_extension_by_nid: ::X509V3_EXT_nconf_nid failed");
		
		return x509_extension_uptr(cert_ext);
	}
	
	void add_extension_by_nid(::X509 * cert, int nid, std::string value)
	{
		auto cert_ext = ::X509V3_EXT_nconf_nid(nullptr, nullptr, nid, value.c_str());
		if (not cert_ext) throw_last_error("ext::openssl::add_extension_by_nid: ::X509V3_EXT_nconf_nid failed");
		
		x509_extension_uptr ext_uptr(cert_ext);
		int res = ::X509_add_ext(cert, cert_ext, -1);
		if (not res) throw_last_error("ext::openssl::add_extension_by_nid: ::X509_add_ext failed");
	}

	::X509_EXTENSION * get_extension_by_nid(const ::stack_st_X509_EXTENSION * extensions, int nid)
	{
		assert(extensions);
		
		int index = ::X509v3_get_ext_by_NID(extensions, nid, -1);
		if (index < 0) return nullptr;
		return ::X509v3_get_ext(extensions, index);
	}
	
	::X509_EXTENSION * get_extension_by_nid(const ::X509 * cert, int nid)
	{
		assert(cert);
		
		auto * extensions = ::X509_get0_extensions(cert);
		if (not extensions) return nullptr;
		
		return get_extension_by_nid(extensions, nid);
	}
	
	std::string print_extension(::X509_EXTENSION * extension)
	{
		assert(extension);
		
		auto * mem_bio = ::BIO_new(::BIO_s_mem());
		if (not mem_bio) throw_last_error("ext::openssl::print_extension: ::BIO_new failed");
		bio_uptr bio_ptr(mem_bio);
		
		int res = ::X509V3_EXT_print(mem_bio, extension, 0, 0);
		if (not res) throw_last_error("ext::openssl::print_extension: ::X509V3_EXT_print failed");
		
		char * data;
		int len = ::BIO_get_mem_data(mem_bio, &data);
		return std::string(data, len);
	}
	
	std::string get_extension_value_by_nid(const ::X509 * cert, int nid)
	{
		auto * extension = get_extension_by_nid(cert, nid);
		if (not extension) return "";
		
		return print_extension(extension);
	}
	
	
	evp_pkey_iptr generate_rsa_key(int bits, unsigned long exponent)
	{
		int res;
		auto * exp = ::BN_new();
		if (not exp) throw_last_error("ext::openssl::generate_rsa_key: exponent ::BN_new failed");
		
		bignum_uptr exp_uptr(exp);
		res = ::BN_set_word(exp, exponent);
		if (not res) throw_last_error("ext::openssl::generate_rsa_key: exponent ::BN_set_word failed");
		
		auto * rsa = ::RSA_new();
		if (not rsa) throw_last_error("ext::openssl::generate_rsa_key: ::RSA_new failed");
		rsa_uptr rsa_uptr(rsa);
		
		res = ::RSA_generate_key_ex(rsa, 4096, exp, nullptr);
		if (not res) throw_last_error("ext::openssl::generate_rsa_key: ::RSA_generate_key_ex failed");
		
		auto * pkey = ::EVP_PKEY_new();
		if (not pkey) throw_last_error("ext::openssl::generate_rsa_key: ::EVP_PKEY_new failed");
		evp_pkey_iptr evp_pkey_iptr(pkey, ext::noaddref);
		
		res = ::EVP_PKEY_assign_RSA(pkey, rsa_uptr.release());
		if (not res) throw_last_error("ext::openssl::generate_rsa_key: ::EVP_PKEY_assign_RSA failed");
		
		return evp_pkey_iptr;
	}
	
	void init_certificate(::X509 * cert, std::string subject, std::string issuer)
	{
		assert(cert);
		int res;
		
		res = ::X509_set_version(cert, 2);
		if (not res) throw_last_error("ext::openssl::init_certificate: ::X509_set_version failed");
		
		//res = ::ASN1_INTEGER_set(::X509_get_serialNumber(cert), 0);
		//if (not res) throw_last_error("ext::openssl::init_certificate: certificate serial ::ASN1_INTEGER_set failed");
		
		auto * cert_name = ::X509_get_subject_name(cert);
		x509_name_add_entry_by_txt(cert_name, std::move(subject));
		
		auto * issuer_name = ::X509_get_issuer_name(cert);
		x509_name_add_entry_by_txt(issuer_name, std::move(issuer));
		
		auto cert_ext = ::X509V3_EXT_nconf_nid(nullptr, nullptr, NID_basic_constraints, "critical, CA:false");
		if (not cert_ext) throw_last_error("ext::openssl::init_certificate: basicConstraints extension ::X509V3_EXT_nconf_nid failed");
		
		x509_extension_uptr ext_uptr(cert_ext);
		res = ::X509_add_ext(cert, cert_ext, -1);
		if (not res) throw_last_error("ext::openssl::init_certificate: basicConstraints extension ::X509_add_ext failed");
	}
	
	void init_self_signed_certificate(::X509 * cert, std::string subject)
	{
		assert(cert);
		int res;
		
		res = ::X509_set_version(cert, 2);
		if (not res) throw_last_error("ext::openssl::init_self_signed_certificate: ::X509_set_version failed");
		
		res = ::ASN1_INTEGER_set(::X509_get_serialNumber(cert), 0);
		if (not res) throw_last_error("ext::openssl::init_self_signed_certificate: certificate serial ::ASN1_INTEGER_set failed");
		
		auto * cert_name = ::X509_get_subject_name(cert);
		x509_name_add_entry_by_txt(cert_name, std::move(subject));
		
		res = ::X509_set_issuer_name(cert, cert_name);
		if (not res) throw_last_error("ext::openssl::init_self_signed_certificate: ::X509_set_issuer_name failed");
		
		auto cert_ext = ::X509V3_EXT_nconf_nid(nullptr, nullptr, NID_basic_constraints, "critical, CA:true");
		if (not cert_ext) throw_last_error("ext::openssl::init_self_signed_certificate: basicConstraints extension ::X509V3_EXT_nconf_nid failed");
		
		x509_extension_uptr ext_uptr(cert_ext);
		res = ::X509_add_ext(cert, cert_ext, -1);
		if (not res) throw_last_error("ext::openssl::init_self_signed_certificate: basicConstraints extension ::X509_add_ext failed");
	}
	
	x509_iptr create_self_signed_certificate(::EVP_PKEY * pkey, std::string subject, std::chrono::seconds duration)
	{
		assert(pkey);
		
		auto * cert = ::X509_new();
		if (not cert) throw_last_error("ext::openssl::create_self_signed_certificate: ::X509_new failed");
		x509_iptr cert_ptr(cert, ext::noaddref);
		
		init_self_signed_certificate(cert, std::move(subject));
		
		::X509_gmtime_adj(::X509_get_notBefore(cert), 0);
		::X509_gmtime_adj(::X509_get_notAfter(cert), duration.count());
		
		int res = ::X509_set_pubkey(cert, pkey);
		if (not res) throw_last_error("ext::openssl::create_self_signed_certificate: ::X509_set_pubkey failed");
		
		return cert_ptr;
	}
	
	x509_iptr create_certificate(::EVP_PKEY * pkey, std::string subject, std::string issuer, std::chrono::seconds duration)
	{
		assert(pkey);
		
		auto * cert = ::X509_new();
		if (not cert) throw_last_error("ext::openssl::create_certificate: ::X509_new failed");
		x509_iptr cert_ptr(cert, ext::noaddref);
		
		init_certificate(cert, std::move(subject), std::move(issuer));
		
		::X509_gmtime_adj(::X509_get_notBefore(cert), 0);
		::X509_gmtime_adj(::X509_get_notAfter(cert), duration.count());
		
		int res = ::X509_set_pubkey(cert, pkey);
		if (not res) throw_last_error("ext::openssl::create_certificate: ::X509_set_pubkey failed");
		
		return cert_ptr;
	}
	
	
	void sign_certificate(::X509 * cert, ::EVP_PKEY * signkey)
	{
		return sign_certificate(cert, signkey, ::EVP_sha256());
	}
	
	void sign_certificate(::X509 * cert, ::EVP_PKEY * signkey, const ::EVP_MD * method)
	{
		assert(cert);
		assert(signkey);
		assert(method);
		
		int res = ::X509_sign(cert, signkey, method);
		if (not res) throw_last_error("ext::openssl::sign_certificate: ::X509_sign failed");
	}
	
}

#if OPENSSL_VERSION_NUMBER < 0x10101000L
static unsigned double_digits_to_uint(const char *& str)
{
	unsigned n = 10 * (*str++ - '0');
	return n + (*str++ - '0');
}

static int ASN1_TIME_to_tm(const ::ASN1_TIME * time, std::tm * tm)
{
	if (not time) return -1;
	const char * str = reinterpret_cast<const char*>(time->data);
	if (not str) return -1;
	
	unsigned year, month, day, hour, min, sec;
	switch(time->type) {
		// https://tools.ietf.org/html/rfc5280#section-4.1.2.5.1
		case V_ASN1_UTCTIME: // YYMMDDHHMMSSZ
			year = double_digits_to_uint(str);
			year += year < 50 ? 2000 : 1900;
			break;
		case V_ASN1_GENERALIZEDTIME: // YYYYMMDDHHMMSSZ
			year  = 100 * double_digits_to_uint(str);
			year +=       double_digits_to_uint(str);
			break;
		default: return -1; // error
	}
	
	month = double_digits_to_uint(str);
	day   = double_digits_to_uint(str);
	hour  = double_digits_to_uint(str);
	min   = double_digits_to_uint(str);
	sec   = double_digits_to_uint(str);
	if (*str != 'Z') return -1;
	
	if (year == 9999 && month == 12 && day == 31 && hour == 23 && min == 59 && sec == 59) // 99991231235959Z rfc 5280
		return -1;
	
	std::memset(tm, 0, sizeof(*tm));
	tm->tm_year = year - 1900;
	tm->tm_mon  = month;
	tm->tm_mday = day;
	tm->tm_hour = hour;
	tm->tm_min  = min;
	tm->tm_sec  = sec;
	
	return 1;
}
#endif

#endif // #ifdef EXT_ENABLE_OPENSSL
