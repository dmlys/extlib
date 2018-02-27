#pragma once
#include <string>
#include <system_error>
#include <boost/predef.h>
#include <boost/system/error_code.hpp>

namespace ext
{
	///формирует сообщение об ошибке в формате
	///<ErrCat>:<ErrCode>, <msg>
	///например
	///generic:0x1, file not found
	std::string FormatError(std::error_code err);
	std::string FormatError(boost::system::error_code err);

	inline
	std::string FormatErrno(int err)
	{
		return FormatError(std::error_code(err, std::generic_category()));
	}

	inline
	void ThrowLastErrno(const std::string & errMsg)
	{
		throw std::system_error(errno, std::generic_category(), errMsg);
	}

	inline
	void ThrowLastErrno(const char * errMsg)
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