#pragma once
#ifdef EXT_ENABLE_OPENSSL
#include <ctime>
#include <memory>
#include <tuple>
#include <ostream>
#include <vector>
#include <string>
#include <string_view>
#include <chrono>
#include <system_error>
#include <fmt/format.h>
#include <openssl/opensslv.h> // for openssl version
#include <ext/intrusive_ptr.hpp>
#include <ext/config.hpp>

// forward some openssl types
typedef struct asn1_string_st ASN1_INTEGER;
typedef struct asn1_string_st ASN1_TIME;


typedef struct bignum_st         BIGNUM;
typedef struct bio_st            BIO;
typedef struct X509_name_st      X509_NAME;
typedef struct X509_extension_st X509_EXTENSION;
typedef struct x509_st           X509;
typedef struct rsa_st            RSA;
typedef struct evp_pkey_st       EVP_PKEY;
typedef struct evp_pkey_ctx_st   EVP_PKEY_CTX;
typedef struct PKCS12_st         PKCS12;
typedef struct evp_md_st         EVP_MD;

struct stack_st_X509;
struct stack_st_X509_EXTENSION;

struct ssl_st;
struct ssl_ctx_st;
struct ssl_method_st;

typedef struct ssl_st        SSL;
typedef struct ssl_ctx_st    SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;


int  intrusive_ptr_add_ref(BIO * ptr);
void intrusive_ptr_release(BIO * ptr);

int  intrusive_ptr_add_ref(::X509 * ptr);
void intrusive_ptr_release(::X509 * ptr);

int  intrusive_ptr_add_ref(::RSA * ptr);
void intrusive_ptr_release(::RSA * ptr);

int  intrusive_ptr_add_ref(::EVP_PKEY * ptr);
void intrusive_ptr_release(::EVP_PKEY * ptr);

int  intrusive_ptr_add_ref(::SSL * ptr);
void intrusive_ptr_release(::SSL * ptr);

int  intrusive_ptr_add_ref(::SSL_CTX * ptr);
void intrusive_ptr_release(::SSL_CTX * ptr);


/// Simple openssl wrappers and utility functions.
/// error category, startup/clean up routines, may be something other
namespace ext::openssl
{
	/// OpenSSL stores it's errors into thread local queue.
	/// If something bad happens more than one error can be pushed into that queue.
	/// That sorta conflicts with last_error mechanism, which can only report one error.
	/// Even more: error can have some associated runtime data, source file name and line number, std::error_code does not supports that at all.
	/// print_error_queue(::ERR_print_errors) functions can print them just fine, whole queue with all data;
	/// so logging/printing errors can be more convenient with those.
	/// ERR_get_error, through, removes error from queue, and ::ERR_print_errors will miss that error
	///
	/// Some of this library functions provide error_retrieve_type enum pointing how to take openssl error: get or peek
	/// By default it's always get, but if you more comfortable with print_error_queue - you can change to peek, and than use one, note that ERR_print_errors clears queue
	enum class error_retrieve : unsigned
	{
		get,  /// use ::ERR_get_error
		peek, /// use ::ERR_peek_error
	};

	// those can be used as err == ext::openssl_error::zero_return.
	// also we do not include openssl/ssl.h, those definition are asserted in ext/openssl.cpp
	enum class ssl_errc
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

	extern const unsigned long rsa_f4; // = RSA_F4/65537
	
	/// error category for openssl errors from ERR_*
	const std::error_category & openssl_err_category() noexcept;
	/// error category for openssl errors from SSL_*
	const std::error_category & openssl_ssl_category() noexcept;

	/// интеграция с system_error
	inline std::error_code make_error_code(ssl_errc val) noexcept           { return {static_cast<int>(val), openssl_ssl_category()}; }
	inline std::error_condition make_error_condition(ssl_errc val) noexcept { return {static_cast<int>(val), openssl_ssl_category()}; }

	/// creates error_code by given sslcode retrieved with SSL_get_error(..., ret)
	/// if sslcode == SSL_ERROR_SYSCALL, checks ERR_get_error() / system error,
	/// if ERR_get_error gives error - uses it and sets openssl_err_category,
	/// else uses openssl_ssl_category
	std::error_code openssl_geterror(int sslcode, error_retrieve rtype = error_retrieve::get) noexcept;

	/// cleans OpenSSL error queue, calls ERR_clear_error
	void openssl_clear_errors() noexcept;

	/// returns last SSL error via ::ERR_get_error()
	std::error_code last_error(error_retrieve rtype = error_retrieve::get) noexcept;
	
	/// throws openssl_error with last_error error_code
	/// if rtype == get, prints whole openssl error queue and places result into thrown exception,
	/// if rtype == peek, does not extract openssl error queue, exception object will hold empty string
	EXT_NORETURN void throw_last_error_str(error_retrieve rtype, const std::string & errmsg);
	
	
	EXT_NORETURN inline void throw_last_error_str(const std::string & errmsg)
	{
		throw_last_error_str(error_retrieve::get, errmsg);
	}
	
	template <class ... Args>
	EXT_NORETURN inline void throw_last_error(error_retrieve rtype, fmt::format_string<> fmt, Args && ... args)
	{
		throw_last_error_str(rtype, fmt::format(fmt, std::forward<Args>(args)...));
	}
	
	template <class ... Args>
	EXT_NORETURN inline void throw_last_error(fmt::format_string<> fmt, Args && ... args)
	{
		throw_last_error_str(fmt::format(fmt, std::forward<Args>(args)...));
	}

	/// prints openssl error queue into string, see ::ERR_print_errors,
	/// error queue will be empty after executing this function
	void print_error_queue(std::string & str);
	/// prints openssl error queue into ostream, see ::ERR_print_errors,
	/// error queue will be empty after executing this function
	void print_error_queue(std::ostream & os);
	/// prints openssl error queue into streambuf, see ::ERR_print_errors,
	/// error queue will be empty after executing this function
	void print_error_queue(std::streambuf & os);
	/// prints openssl error queue into string and returns it, see ::ERR_print_errors,
	/// error queue will be empty after executing this function
	std::string print_error_queue();
	
	/// Exception for openssl error, in addition to std::system_error holds openssl error queue
	/// This class does not call print_error_queue in any way, in just holds additional string, see throw_last_error
	class openssl_error : public std::system_error
	{
	protected:
		std::string m_error_queue;
		
	public:
		std::string error_queue() const { return m_error_queue; }
		
	public:
		using system_error::system_error;
		
		openssl_error(std::error_code ec, const std::string & what, std::string error_queue)
		    : std::system_error(ec, what), m_error_queue(std::move(error_queue)) {}
	};
	
	/// per process initialization.
	/// Calls ERR_load_crypto_strings for older versions of library, does nothing for modern versions.
	void crypto_init();
	/// per process initialization.
	/// Calls SSL_load_error_strings, SSL_library_init for older versions of library, does nothing for modern versions.
	void ssl_init();
	/// per process ssl/crypto cleanup
	void lib_cleanup();


	// some smart pointers helpers
	struct ssl_deleter      { void operator()(::SSL * ssl)        const noexcept; };
	struct ssl_ctx_deleter  { void operator()(::SSL_CTX * sslctx) const noexcept; };

	struct bignum_deleter   { void operator()(::BIGNUM * bn)     const noexcept; };
	struct bio_deleter      { void operator()(::BIO * bio)       const noexcept; };
	struct x509_deleter     { void operator()(::X509 * cert)     const noexcept; };
	struct rsa_deleter      { void operator()(::RSA * rsa)       const noexcept; };
	struct evp_pkey_deleter { void operator()(::EVP_PKEY * pkey) const noexcept; };
	struct pkcs12_deleter   { void operator()(::PKCS12 * pkcs12) const noexcept; };
	
	struct x509_extension_deleter { void operator()(::X509_EXTENSION * extension) const noexcept; };
	
	struct evp_pkey_ctx_deleter { void operator()(::EVP_PKEY_CTX * ctx) const noexcept; };
	struct stackof_x509_deleter { void operator()(::stack_st_X509 * x590s) const noexcept; };
	struct stack_st_x509_extension_deleter { void operator()(::stack_st_X509_EXTENSION * extensions) const noexcept; };

	using ssl_uptr     = std::unique_ptr<::SSL, ssl_deleter>;
	using ssl_ctx_uptr = std::unique_ptr<::SSL_CTX, ssl_ctx_deleter>;

	using bignum_uptr   = std::unique_ptr<::BIGNUM, bignum_deleter>;
	using bio_uptr      = std::unique_ptr<::BIO, bio_deleter>;
	using x509_uptr     = std::unique_ptr<::X509, x509_deleter>;
	using rsa_uptr      = std::unique_ptr<::RSA, rsa_deleter>;
	using evp_pkey_uptr = std::unique_ptr<::EVP_PKEY, evp_pkey_deleter>;
	using pkcs12_uptr   = std::unique_ptr<::PKCS12, pkcs12_deleter>;

	using evp_pkey_ctx_uptr   = std::unique_ptr<::EVP_PKEY_CTX, evp_pkey_ctx_deleter>;
	using x509_extension_uptr = std::unique_ptr<::X509_EXTENSION, x509_extension_deleter>;
	using stackof_x509_uptr   = std::unique_ptr<::stack_st_X509, stackof_x509_deleter>;
	
	using stack_st_x509_extension_uptr = std::unique_ptr<::stack_st_X509_EXTENSION, stack_st_x509_extension_deleter>;


	using ssl_iptr      = ext::intrusive_ptr<::SSL>;
	using ssl_ctx_iptr  = ext::intrusive_ptr<::SSL_CTX>;

	using bio_iptr      = ext::intrusive_ptr<::BIO>;
	using x509_iptr     = ext::intrusive_ptr<::X509>;
	using rsa_iptr      = ext::intrusive_ptr<::RSA>;
	using evp_pkey_iptr = ext::intrusive_ptr<::EVP_PKEY>;

	/************************************************************************/
	/*                     print and date helpers                           */
	/************************************************************************/
	
	/// prints name with X509_NAME_print_ex and given flags
	std::string x509_name_string(const ::X509_NAME * name, int flags);
	/// prints name with X509_NAME_print_ex and flags:
	///  (XN_FLAG_RFC2253 | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB
	std::string x509_name_string(const ::X509_NAME * name);
	/// prints name with X509_NAME_print_ex and flags:
	///  (XN_FLAG_DN_REV | XN_FLAG_RFC2253 | XN_FLAG_SEP_CPLUS_SPC) & ~XN_FLAG_SEP_COMMA_PLUS & ~ASN1_STRFLGS_ESC_MSB;
	std::string x509_name_reversed_string(const ::X509_NAME * name);
	
	/// prints num via BN_bn2dec
	std::string bignum_string(const ::BIGNUM * num);
	/// converts asn1 integer to bignum and prints it
	std::string asn1_integer_string(const ::ASN1_INTEGER * integer);
	/// converts asn1 time to time_t, then prints via std::strftime(..., "%c", ...)
	/// %c - locale specific standard date and time string
	/// see also asn1_time_print, asn1_time_timet
	std::string asn1_time_string(const ::ASN1_TIME * time);

	/// Converts time to time_t type, with help of asn1_time_tm and std::mkgmtime(or platform alternative)
	/// returns -1 for special invalid date value
	/// SOME REMARKS:
	///  ASN1_TIME - just a ASN1_STRING, which holds date as string constrained by RFC 5280
	///  by that RFC date/time should stored in:
	///   * UTCTime: date string in format YYMMDDHHMMSSZ, GMT timezone only
	///   * GeneralizedTime: date string YYYYMMDDHHMMSSZ, GMT timezone only
	///  NOTE: special value 99991231235959Z means invalid date
	time_t asn1_time_timet(const ::ASN1_TIME * time);
	/// ASN1_TIME_to_tm wrapper
	std::tm asn1_time_tm(const ::ASN1_TIME * time);
	/// ASN1_TIME_print wrapper
	std::string asn1_time_print(const ::ASN1_TIME * time);
	/// ASN1_TIME_set wrapper
	::ASN1_TIME * asn1_time_set(::ASN1_TIME * time, time_t tt);
	
	
	/************************************************************************/
	/*         write/load certificate/private key/PKCS12 functions          */
	/************************************************************************/
	
	/// Loads X509 certificate from given memory location and with optional password(password probably will never be used).
	/// Throws std::system_error in case of errors
	x509_iptr     load_certificate(const char * data, std::size_t len, std::string_view passwd = "");
	// loads private key from given memory location and with optional password
	/// Throws std::system_error in case of errors
	evp_pkey_iptr load_private_key(const char * data, std::size_t len, std::string_view passwd = "");

	inline x509_iptr     load_certificate(std::string_view str, std::string_view passwd = "") { return load_certificate(str.data(), str.size(), passwd); }
	inline evp_pkey_iptr load_private_key(std::string_view str, std::string_view passwd = "") { return load_private_key(str.data(), str.size(), passwd); }
	
	/// Writes X509 certificate into memory in PEM format and returns string holding it.
	/// Throws std::system_error in case of errors
	std::string write_certificate(const ::X509 * cert);
	/// Writes private key into memory in PEM format and returns string holding it.
	/// Key will be unprotected(no password encryption).
	/// Throws std::system_error in case of errors
	std::string write_pkey(const ::EVP_PKEY * key);

	/// Loads X509 certificate from given path and with optional password
	/// Throws std::system_error in case of errors
	x509_iptr load_certificate_from_file(const char * path, std::string_view passwd = "");
	x509_iptr load_certificate_from_file(std::FILE * file, std::string_view passwd = "");

	/// loads private key from given given path and with optional password
	/// Throws std::system_error in case of errors
	evp_pkey_iptr load_private_key_from_file(const char * path, std::string_view passwd = "");
	evp_pkey_iptr load_private_key_from_file(std::FILE * path, std::string_view passwd = "");

	/// Writes X509 certificate into given file in PEM format.
	/// Throws std::system_error in case of errors
	void write_certificate_to_file(std::FILE * fp, ::X509 * cert);
	void write_certificate_to_file(const char * fname, ::X509 * cert);
	
	/// Writes private key into given file in PEM format.
	/// Key will be unprotected(no password encryption).
	/// Throws std::system_error in case of errors
	void write_pkey_to_file(std::FILE * fp, const ::EVP_PKEY * pkey);
	void write_pkey_to_file(const char * fname, const ::EVP_PKEY * pkey);
	
	
	/// Loads PKCS12 file from given memory location.
	/// Throws std::system_error in case of errors
	pkcs12_uptr load_pkcs12(const char * data, std::size_t len);
	/// Loads PKCS12 file from given path.
	/// Throws std::system_error in case of errors
	pkcs12_uptr load_pkcs12_from_file(const char * path);
	pkcs12_uptr load_pkcs12_from_file(std::FILE * file);
	
	/// Writes PKCS12 into memory in DER format and returns it.
	/// Throws std::system_error in case of errors
	std::vector<char> write_pkcs12(const ::PKCS12 * pkcs12);
	/// Writes PKCS12 into given file in DER format.
	/// Passwork protection is set by other functions, this is just a serializtion function.
	/// Throws std::system_error in case of errors
	void write_pkcs12_to_file(std::FILE * fp, const ::PKCS12 * pkcs12);
	void write_pkcs12_to_file(const char * path, const ::PKCS12 * pkcs12);

	inline pkcs12_uptr load_pkcs12(std::string_view str) { return load_pkcs12(str.data(), str.size()); }

	inline x509_iptr     load_certificate_from_file(const std::string & path, std::string_view passwd = "") { return load_certificate_from_file(path.c_str(), passwd); }
	inline evp_pkey_iptr load_private_key_from_file(const std::string & path, std::string_view passwd = "") { return load_private_key_from_file(path.c_str(), passwd); }
	inline pkcs12_uptr   load_pkcs12_from_file(const std::string & path) { return load_pkcs12_from_file(path.c_str()); }
	
	/// Tests if PKCS12 is password encrypted, basicly a PKCS12_verify_mac wrapper.
	/// NOTE: null password and empty password for PKCS12 encryption are 2 different things.
	///       This functions for empty password tries both cases: null and "", if one succeeds - result is true
	bool pkcs12_password_encrypted(::PKCS12 * p12, std::string_view pass = "");
	
	/// Parses PKCS12 into private key, x509 certificate and certificate authorities
	/// Throws std::system_error in case of errors
	void parse_pkcs12(::PKCS12 * pkcs12, const char * passwd, evp_pkey_iptr & evp_pkey, x509_iptr & x509, stackof_x509_uptr & ca);
	auto parse_pkcs12(::PKCS12 * pkcs12, const char * passwd = "") -> std::tuple<evp_pkey_iptr, x509_iptr, stackof_x509_uptr>;

	/// PKCS12_create wrapper. see man page PKCS12_CREATE(3)
	/// Creates new PKCS12 structure encrypted with given password.
	///   name - friendlyName to use for the supplied certificate and key.
	///   pkey and cert - private key and corresponding certificate to include.
	///   ca, if not NULL, is an optional set of certificates to also include in the structure.
	/// 
	///   nid_key and nid_cert are the encryption algorithms that should be used for the key and certificate respectively.
	///     The modes GCM, CCM, XTS, and OCB are unsupported.
	///     iter is the encryption algorithm iteration count to use and mac_iter is the MAC iteration count to use.
	/// 
	///   keytype is the type of key. keytype adds a flag to the store private key.
	///     This is a non standard extension that is only currently interpreted by MSIE.
	///     If set to zero the flag is omitted, if set to KEY_SIG the key can be used for signing only,
	///     if set to KEY_EX it can be used for signing and encryption.
	///     This option was useful for old export grade software which could use signing only keys of arbitrary size
	///     but had restrictions on the permissible sizes of keys which could be used for encryption.
	/// Throws std::system_error in case of errors
	pkcs12_uptr create_pkcs12(const char * pass, const char * name,
	                          ::EVP_PKEY * pkey, ::X509 * cert, ::stack_st_X509 * ca = nullptr,
	                          int nid_key = 0, int nid_cert = 0, int iter = 0, int mac_iter = 0, int keytype = 0);
	
	/************************************************************************/
	/*                           TLS/SSL stuff                              */
	/************************************************************************/
	
	/// creates SSL_CTX with given SSL method and sets given certificate and private key and CA chain(SSL_CTX_use_cert_and_key)
	ssl_ctx_iptr create_sslctx(const ::SSL_METHOD * method, ::X509 * cert, ::EVP_PKEY * pkey, ::stack_st_X509 * ca_chain = nullptr);
	/// creates SSL_CTX with SSLv23_server_method and sets given certificate, private key and CA chain(SSL_CTX_use_cert_and_key)
	ssl_ctx_iptr create_sslctx(::X509 * cert, ::EVP_PKEY * pkey, ::stack_st_X509 * ca_chain = nullptr);

	/// creates SSL_CTX with given SSL method; sets cipher LIST to "aNULL,eNULL"
	/// which is alias for "The cipher suites offering no authentication."
	/// see https://www.openssl.org/docs/man1.1.0/apps/ciphers.html,
	/// also invokes SSL_CTX_set_tmp_dh with result of DH_get_2048_256.
	///
	/// NOTE: this is INSECURE and should not be used at all,
	///       but allows establishing connection without certificates.
	///       most clients have those ciphers disabled by default
	ssl_ctx_iptr create_anonymous_sslctx(const ::SSL_METHOD * method);
	///	same as above with SSLv23_server_method
	ssl_ctx_iptr create_anonymous_sslctx();
	
	/************************************************************************/
	/*                certificate attributes/extensions                     */
	/************************************************************************/
	
	/// Sets not before property of cert to given time point
	void set_notbefore(::X509 * cert, std::chrono::system_clock::time_point tpoint);
	/// Sets not after property of cert to given time point
	void set_notafter(::X509 * cert, std::chrono::system_clock::time_point tpoint);
	/// Sets certificate duration, basicly takes not before, adds duration, sets result into not after
	void set_duration(::X509 * cert, std::chrono::system_clock::duration duration);
	/// Gets certificate not before property as std::chrono::system_clock::time_point
	auto get_notbefore(const ::X509 * cert) -> std::chrono::system_clock::time_point;
	/// Gets certificate not after property as std::chrono::system_clock::time_point
	auto get_notafter(const ::X509 * cert) -> std::chrono::system_clock::time_point;
	
	/// Calculates and returns certificate SHA1 fingerprint, basicly ::X509_digest wrapper
	/// Throws std::system_error in case of errors
	std::vector<unsigned char> cert_sha1fingerprint(const ::X509 * cert);
	
	/// Wrapper around X509_NAME_add_entry_by_txt:
	///   X509_NAME_add_entry_by_txt(cert_name, name.c_str(), MBSTRING_UTF8, value.data(), value.size(), -1, 0);
	/// Throws std::system_error in case of errors
	void x509_name_add_entry_by_txt(::X509_NAME * cert_name, const std::string & name, std::string_view value);
	/// Similar to OpenSSL X509_NAME_add_entry_by_txt, but name is complete x509 name,
	/// which splited into parts delimited by comma and space chars, each part then added via X509_NAME_add_entry_by_txt.
	/// For expample: C=CA, O="MyCompamy, friends and \"Me\"", CN=localhost
	/// produces:
	///   * X509_NAME_add_entry_by_txt 'C', 'CA'
	///   * X509_NAME_add_entry_by_txt 'O', '"MyCompamy, friends and \"Me\""'
	///   * X509_NAME_add_entry_by_txt 'CN', 'localhost'
	/// Throws std::system_error in case of errors
	void x509_name_add_entry_by_txt(::X509_NAME * cert_name, std::string name_str);
	
	/// wrapper around X509V3_EXT_nconf_nid
	/// value is same as used in openssl.conf
	/// NOTE: X509V3_EXT_nconf_nid is not documented API, but it is used by OpenSSL internally to create v3 extensions
	/// Throws std::system_error in case of errors
	x509_extension_uptr create_extension_by_nid(int nid, std::string value);
	/// creates extension and adds it to given cert, basicly wrapper around X509V3_EXT_nconf_nid and X509_add_ext.
	/// value is same as used in openssl.conf
	/// NOTE: X509V3_EXT_nconf_nid is not documented API, but it is used by OpenSSL internally to create v3 extensions
	/// Throws std::system_error in case of errors
	void add_extension_by_nid(::X509 * cert, int nid, std::string value);
	
	/// Gets extension from certificate by given NID.
	/// Returns null if not found.
	/// Throws std::system_error in case of errors
	::X509_EXTENSION * get_extension_by_nid(const ::stack_st_X509_EXTENSION * extensions, int nid);
	::X509_EXTENSION * get_extension_by_nid(const ::X509 * cert, int nid);
	/// Prints given extension value, basicly X509V3_EXT_print wrapper
	/// Throws std::system_error in case of errors
	std::string print_extension(::X509_EXTENSION * extension);
	/// Prints and returns extension by NID from certificate,
	/// basicly get_extension_by_nid + print_extension
	/// Throws std::system_error in case of errors
	std::string get_extension_value_by_nid(const ::X509 * cert, int nid);
	
	/************************************************************************/
	/*                Private Key/Certificate generation                    */
	/************************************************************************/
	/// Generates RSA keys with given bites and exponent,
	/// Basicly it's a wrapper around RSA_generate_key_ex
	/// Throws std::system_error in case of errors
	evp_pkey_iptr generate_rsa_key(int bits, unsigned long exponent = rsa_f4);
	
	/// Inits empty X509 certificate, sets:
	///  * given subject
	///  * given issuer
	///  * version 3(2)
	/// Throws std::system_error in case of errors
	void init_certificate(::X509 * cert, std::string subject, std::string issuer);
	/// Inits empty X509 certificate, sets:
	///  * given subject
	///  * issuer name = subject
	///  * version 3(2)
	///  * serial number = 0
	///  * adds extension basicContraint = critical, CA:TRUE
	/// Throws std::system_error in case of errors
	void init_self_signed_certificate(::X509 * cert, std::string subject);
	
	/// creates new certificate and sets pubkey, inits it via init_self_signed_certificate, configures:
	///  * not before = current timestamp
	///  * not after = current timestamp + duration
	/// Does not signs it
	/// Throws std::system_error in case of errors
	x509_iptr create_self_signed_certificate(::EVP_PKEY * pkey, std::string subject, std::chrono::seconds duration = std::chrono::seconds(12 * 30 * 24 * 60 * 60)/*one year*/);
	/// creates new certificate and sets pubkey, inits it via init_certificate, configures:
	///  * not before = current timestamp
	///  * not after = current timestamp + duration
	/// Does not signs it
	/// Throws std::system_error in case of errors
	x509_iptr create_certificate(::EVP_PKEY * pkey, std::string subject, std::string issuer, std::chrono::seconds duration = std::chrono::seconds(12 * 30 * 24 * 60 * 60)/*one year*/);
	
	/// signs certificate with given private key, hash method is EVP_sha256()
	/// Throws std::system_error in case of errors
	void sign_certificate(::X509 * cert, ::EVP_PKEY * signkey);
	/// signs certificate with given private key and hash method.
	/// Throws std::system_error in case of errors
	void sign_certificate(::X509 * cert, ::EVP_PKEY * signkey, const ::EVP_MD * method);
}

namespace ext
{
	// because those have openssl in theirs names - bring them to ext namespace
	using openssl::openssl_err_category;
	using openssl::openssl_ssl_category;
	using openssl::openssl_geterror;
	
	using openssl_errc  = openssl::ssl_errc;
	using openssl_error = openssl::openssl_error;
	
}

namespace std
{
	template <>
	struct is_error_code_enum<ext::openssl::ssl_errc>
		: std::true_type { };
}

#endif // EXT_ENABLE_OPENSSL
