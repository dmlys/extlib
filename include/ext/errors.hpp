#pragma once
#include <string>
#include <system_error>
#include <ext/config.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace ext
{
	/// formats error message like: <ErrCat>:<ErrCode>, <msg>
	/// for example: generic:0x1, file not found
	std::string format_error(std::error_code err);
	std::string format_error(boost::system::error_code err);

	/// re-formats error message std::system_error::what(), so it contains
	/// code description in format: <ErrCat>:<ErrCode>, <msg>
	std::string format_error(std::system_error & err);
	std::string format_error(boost::system::system_error err);

	inline std::string format_errno(int err) { return format_error(std::error_code(err, std::generic_category())); }
	
	/// returns last system error:
	///   on windows - GetLastError()
	///   on posix   - errno
	std::error_code last_system_error() noexcept;
	
	/// In windows system_category implemented via FormatMessageA, as result
	/// error text is returned localized in some local one-byte code page
	///
	/// This category returns text with MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) in ASCII(or should be)
	/// On other OS - this is same as std::system_category
	std::error_category const & system_utf8_category() noexcept;
	boost::system::error_category const & boost_system_utf8_category() noexcept;
	
	/// equivalent throw std::system_error(last_system_error, errMsg);
	EXT_NORETURN inline void throw_last_system_error(const char * errmsg)         { throw std::system_error(last_system_error(), errmsg); }
	EXT_NORETURN inline void throw_last_system_error(const std::string & errmsg)  { throw std::system_error(last_system_error(), errmsg); }
	
	EXT_NORETURN inline void throw_last_errno(const std::string & errmsg) { throw std::system_error(errno, std::generic_category(), errmsg); }
	EXT_NORETURN inline void throw_last_errno(const char * errmsg)        { throw std::system_error(errno, std::generic_category(), errmsg); }
}
