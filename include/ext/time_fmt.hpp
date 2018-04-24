#pragma once
#include <ctime>
#include <string>
#include <boost/predef.h>

namespace ext
{
	/// преобразует time_t к iso 8601 формату
	/// YYYY-mm-ddTHH:MM:SS. 2014-03-21T03:55:05
	std::string to_isodate(const std::tm * t);
	std::string to_isodate(std::time_t tpoint);

	/// преобразует time_t к iso 8601 формату
	/// YYYYmmddTHHMMSS. 20140321T035505
	std::string to_isodate_undelimeted(const std::tm * t);
	std::string to_isodate_undelimeted(std::time_t tpoint);

	/// chrono overloads

	template <class time_point, class = decltype(time_point::clock::to_time_t)>
	inline std::string to_isodate(time_point tpoint)
	{
		return to_isodate(time_point::clock::to_time_t(tpoint));
	}

	template <class time_point, class = decltype(time_point::clock::to_time_t)>
	inline std::string to_isodate_undelimeted(time_point tpoint)
	{
		return to_isodate_undelimeted(time_point::clock::to_time_t(tpoint));
	}



	using std::localtime;
	using std::gmtime;
	using std::mktime;

	/// версия gmtime без использование глобальных переменных
	inline void gmtime(const time_t * tpoint, std::tm * t)
	{
		#if BOOST_OS_WINDOWS
			gmtime_s(t, tpoint);
		#elif BOOST_OS_LINUX || BOOST_OS_UNIX
			gmtime_r(tpoint, t);
		#else
			#error(not implemented for this platform)
		#endif
	}

	/// версия localtime без использование глобальных переменных
	inline void localtime(const std::time_t * tpoint, std::tm * t)
	{
		#if BOOST_OS_WINDOWS
			localtime_s(t, tpoint);
		#elif BOOST_OS_LINUX || BOOST_OS_UNIX
			localtime_r(tpoint, t);
		#else
			#error(not implemented for this platform)
		#endif
	}

	/// GMT mktime версия
	inline time_t mkgmtime(std::tm * t)
	{
		#if BOOST_OS_WINDOWS
			return _mkgmtime(t);
		#elif BOOST_OS_HPUX
			// ugly, does not account daylight saving
			return mktime(t) - timezone;
		#elif BOOST_OS_LINUX || BOOST_OS_UNIX
			return timegm(t);
		#else
			// ugly, does not account daylight saving
			return mktime(t) - timezone;
		#endif
	}
}
