#pragma once
#include <type_traits>
#include <system_error>

namespace ext
{
	/// socket error condition, this is NOT error codes,
	/// this is convenience conditions socket can be tested on.
	///
	/// you should use them like:
	///  * if (ss.last_error() == sock_errc::error) std::cerr << ss.last_error ...
	///  * if (ss.last_error() == sock_errc::regular) { ... process result ... } 
	enum class sock_errc
	{
		eof      = 1,   /// socket eof, for example recv return 0, or OpenSSL returned SSL_ERR_ZERO_RETURN
		
		regular  = 2,   /// no a error, code == 0 or some error which is not critical, like eof(currently only eof)		
		error    = 3,   /// opposite of regular, some bad unexpected error, which breaks normal flow, timeout, system error, etc
	};

	const std::error_category & socket_condition_category() noexcept;

	/// integration with system_error
	inline std::error_code make_error_code(sock_errc val) noexcept           { return {static_cast<int>(val), socket_condition_category()}; }
	inline std::error_condition make_error_condition(sock_errc val) noexcept { return {static_cast<int>(val), socket_condition_category()}; }
}

namespace std
{
	template <>
	struct is_error_condition_enum<ext::sock_errc>
		: std::true_type {};
}
