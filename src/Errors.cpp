#include <ext/Errors.hpp>
#include <ext/itoa.hpp>

namespace ext
{
	namespace
	{
		template <class ErrCode>
		std::string FormatErrorImpl(const ErrCode & err)
		{
			static_assert(std::is_same<decltype(err.value()), int>::value, "error code should be int");
			std::string msg;

			int errc = err.value();
			auto & cat = err.category();

			// +2 for sign and null terminator
			const std::size_t bufsize = std::numeric_limits<int>::digits10 + 3;
			char buffer[bufsize];
			const char * start = ext::unsafe_itoa(errc, buffer, bufsize, errc & 0x80000000 ? 16 : 10);
			const char * end = buffer + bufsize - 1; // -1 to exclude null terminator

			msg += cat.name();
			msg += ':';
			msg.append(start, end);
			msg += ", ";
			msg += cat.message(errc);

			return msg;
		}
	}

	std::string FormatError(std::error_code err)
	{
#if BOOST_OS_WINDOWS
		if (err.category() == std::system_category())
			err.assign(err.value(), ext::system_utf8_category());
#endif

		return FormatErrorImpl(err);
	}

	std::string FormatError(boost::system::error_code err)
	{
#if BOOST_OS_WINDOWS
		if (err.category() == boost::system::system_category())
			err.assign(err.value(), ext::boost_system_utf8_category());
#endif

		return FormatErrorImpl(err);
	}
}

/************************************************************************/
/*                    Windows specific part                             */
/************************************************************************/
#if BOOST_OS_WINDOWS
#include <windows.h>
#include <vector>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>

namespace ext
{
	namespace
	{
		std::string GetMessage(int ev)
		{
			//http://msdn.microsoft.com/en-us/library/windows/desktop/ms679351%28v=vs.85%29.aspx
			//64k from MSDN: "The output buffer cannot be larger than 64K bytes."
			const size_t bufSz = 64 * 1024;
			wchar_t errBuf[bufSz];
			//std::unique_ptr<wchar_t[]> errBufPtr(new wchar_t[bufSz]);
			//auto errBuf = errBufPtr.get();

			DWORD res = ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			                             nullptr, ev,
			                             MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			                             errBuf, bufSz,
			                             nullptr);
			if (!res)
				return "FormatMessageW failed with " + std::to_string(GetLastError());
			else {
				std::string err = boost::locale::conv::utf_to_utf<char>(errBuf, errBuf + res);
				/// message often contain "\r\n." at the end, delete them
				boost::trim_right_if(err, boost::is_any_of("\r\n."));
				return err;
			}
		}

		struct SystemUtf8Category : std::error_category
		{
			std::string message(int err) const override;
			const char * name() const noexcept override {return "system";}

			std::error_condition default_error_condition(int code) const noexcept override;
		};

		struct BoostSystemUtf8Category : boost::system::error_category
		{
			std::string message(int err) const override;
			const char * name() const noexcept override {return "system";}

			boost::system::error_condition default_error_condition(int code) const noexcept override;
		};

		std::string SystemUtf8Category::message(int err) const
		{
			return GetMessage(err);
		}

		std::error_condition SystemUtf8Category::default_error_condition(int code) const noexcept
		{
			// forward to standard implementation
			return std::system_category().default_error_condition(code);
		}

			std::string BoostSystemUtf8Category::message(int err) const
		{
			return GetMessage(err);
		}

		boost::system::error_condition BoostSystemUtf8Category::default_error_condition(int code) const noexcept
		{
			// forward to standard implementation
			return boost::system::system_category().default_error_condition(code);
		}

		const SystemUtf8Category SystemUtf8CategoryInstance {};
		const BoostSystemUtf8Category BoostSystemUtf8CategoryInstance {};
	} //anonymous namespace

	std::error_category const & system_utf8_category() noexcept
	{
		return SystemUtf8CategoryInstance;
	}

	boost::system::error_category const & boost_system_utf8_category() noexcept
	{
		return BoostSystemUtf8CategoryInstance;
	}

	void ThrowLastSystemError(const std::string & errMsg)
	{
		throw std::system_error(GetLastError(), std::system_category(), errMsg);
	}

	void ThrowLastSystemError(const char * errMsg)
	{
		throw std::system_error(GetLastError(), std::system_category(), errMsg);
	}
}
#endif