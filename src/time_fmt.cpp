#include <ext/time_fmt.hpp>

namespace ext
{
	namespace
	{
		static std::string to_isodate(const std::tm * t)
		{
			const int bufsz = 32;
			char buffer[bufsz];
			auto sz = std::strftime(buffer, bufsz, "%Y-%m-%dT%H:%M:%S", t);
			return {buffer, sz};
		}

		static std::string to_isodate_undelimeted(const std::tm * t)
		{
			const int bufsz = 32;
			char buffer[bufsz];
			auto sz = std::strftime(buffer, bufsz, "%Y%m%dT%H%M%S", t);
			return {buffer, sz};
		}

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
}