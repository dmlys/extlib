#include <ext/time.hpp>

namespace ext
{
	std::string to_isodate(const std::tm * t)
	{
		const int bufsz = 32;
		char buffer[bufsz];
		auto sz = std::strftime(buffer, bufsz, "%Y-%m-%dT%H:%M:%S", t);
		return {buffer, sz};
	}

	std::string to_isodate_undelimeted(const std::tm * t)
	{
		const int bufsz = 32;
		char buffer[bufsz];
		auto sz = std::strftime(buffer, bufsz, "%Y%m%dT%H%M%S", t);
		return {buffer, sz};
	}

	std::string to_isodate(std::time_t tpoint)
	{
		std::tm t;
		localtime(&tpoint, &t);
		return to_isodate(&t);
	}

	std::string to_isodate_undelimeted(std::time_t tpoint)
	{
		std::tm t;
		localtime(&tpoint, &t);
		return to_isodate_undelimeted(&t);
	}
} // namespace ext

#if BOOST_OS_WINDOWS
#include <windows.h>

namespace ext
{
	void make_filetime(std::time_t time, FILETIME * filetime)
	{
		// code taken from MSDN: https://docs.microsoft.com/en-us/windows/win32/sysinfo/converting-a-time-t-value-to-a-file-time
#if not defined (_USE_32BIT_TIME_T)
		ULARGE_INTEGER time_value;
		time_value.QuadPart = (time * 10000000ULL) + 116444736000000000ULL;
		filetime->dwLowDateTime  = time_value.LowPart;
		filetime->dwHighDateTime = time_value.HighPart;
#else
		ULONGLONG time_value = Int32x32To64(time, 10000000) + 116444736000000000ULL;
		filetime->dwLowDateTime  = static_cast<DWORD>(time_value);
		filetime->dwHighDateTime = time_value >> 32;
#endif
	
	}
	
	std::time_t from_filetime(const FILETIME * filetime)
	{
#if not defined(_USE_32BIT_TIME_T)
		ULARGE_INTEGER time_value;
		time_value.LowPart  = filetime->dwLowDateTime;
		time_value.HighPart = filetime->dwHighDateTime;
		return (time_value.QuadPart - 116444736000000000LL) / 10000000LL;
#else
		ULONGLONG time_value = (static_cast<LONGLONG>(filetime->dwHighDateTime) << 32) + filetime->dwLowDateTime;
		return static_cast<std::time_t>((time_value - 116444736000000000LL) / 10000000LL);
#endif
	}
}
#endif
