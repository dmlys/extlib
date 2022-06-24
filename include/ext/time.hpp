#pragma once
#include <ctime>
#include <string>
#include <boost/predef.h>

namespace ext
{
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
	
	/// Convience function for creating std::tm from given year, month, ...
	/// values sjould be given as in normal notation: 2022, 1, 1, 0, 0, 0
	///                                        means: year 2022, 1st January 00:00:00
	inline void make_timepoint(std::tm * t, unsigned year, unsigned month = 1, unsigned day = 1, unsigned hour = 0, unsigned minute = 0, unsigned seconds = 0)
	{
		t->tm_year = year - 1900;
		t->tm_mon  = month - 1;
		t->tm_mday = day;
		t->tm_hour = hour;
		t->tm_min  = minute;
		t->tm_sec  = seconds;
		t->tm_isdst = -1; // negative value means that mktime() should (use timezone information and system databases to)
		                  // attempt to determine whether DST is in effect at the specified time.
	}
	
	inline void make_timepoint(std::time_t & t, unsigned year, unsigned month = 1, unsigned day = 1, unsigned hour = 0, unsigned minute = 0, unsigned seconds = 0)
	{
		std::tm tm;
		make_timepoint(&tm, year, month, day, hour, minute, seconds);
		t = std::mktime(&tm);
	}
	
	template <class time_point, class = decltype(time_point::clock::from_time_t)>
	inline void make_timepoint(time_point & point, unsigned year, unsigned month = 1, unsigned day = 1, unsigned hour = 0, unsigned minute = 0, unsigned seconds = 0)
	{
		std::time_t t;
		make_timepoint(t, year, month, day, hour, minute, seconds);
		point = time_point::clock::from_time_t(t);
	}
	
	template <class time_point, class = decltype(time_point::clock::from_time_t)>
	inline time_point make_timepoint(unsigned year, unsigned month = 1, unsigned day = 1, unsigned hour = 0, unsigned minute = 0, unsigned seconds = 0, unsigned millis = 0)
	{
		std::time_t t;
		make_timepoint(t, year, month, day, hour, minute, seconds);
		return time_point::clock::from_time_t(t);
	}
	
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
	
} // namespace ext

#if BOOST_OS_WINDOWS
typedef struct _FILETIME FILETIME;

namespace ext
{
	/// Converts time_t to WinAPI FILETIME
	void make_filetime(std::time_t time, FILETIME * filetime);
	/// Converts WinAPI FILETIME to time_t
	auto from_filetime(const FILETIME * filetime) -> std::time_t;
}
#endif

