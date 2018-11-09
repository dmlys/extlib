#pragma once
#include <string>
#include <system_error>
#include <boost/predef.h>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace ext
{
	/// formats error message like: <ErrCat>:<ErrCode>, <msg>
	/// for example: generic:0x1, file not found
	std::string FormatError(std::error_code err);
	std::string FormatError(boost::system::error_code err);

	/// re-formats error message std::system_error::what(), so it contains
	/// code description in format: <ErrCat>:<ErrCode>, <msg>
	std::string FormatError(std::system_error & err);
	std::string FormatError(boost::system::system_error err);

	inline std::string FormatErrno(int err)
	{
		return FormatError(std::error_code(err, std::generic_category()));
	}

	inline void ThrowLastErrno(const std::string & errMsg)
	{
		throw std::system_error(errno, std::generic_category(), errMsg);
	}

	inline void ThrowLastErrno(const char * errMsg)
	{
		throw std::system_error(errno, std::generic_category(), errMsg);
	}
}

#if BOOST_OS_WINDOWS
namespace ext
{
	/// в windows system_category реализована через FormatMessageA, как следствие
	/// текст ошибки возвращается в некой местной однобайтовой кодировке + еще и локализованный.
	///
	/// Данные категории возвращают текст в utf-8 и MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
	/// Фактически должен быть ASCII
	std::error_category const & system_utf8_category() noexcept;
	boost::system::error_category const & boost_system_utf8_category() noexcept;

	///эквивалентно throw std::system_error(GetLastError(), std::system_category(), errMsg);
	void ThrowLastSystemError(const char * errMsg);
	void ThrowLastSystemError(const std::string & errMsg);
}
#endif
