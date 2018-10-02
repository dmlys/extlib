#include <ext/regex_utils.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>

namespace ext
{
	namespace
	{
		// следующие символы должны быть экранированы
		// .[{}()\*+?|^$
		// "[.\\[\\]{}()\\\\*+?|^$]"
		static const boost::regex escape_symbols("[.\\[\\]{}()\\\\*+?|^$]");
		// заменяем на \<найденное>
		// $& - full expr match placeholder
		static const std::string format = "\\\\$&";
	}

	std::string escape_regex(const std::string & regx)
	{
		return boost::replace_all_regex_copy(regx, escape_symbols, format);
	}

	bool is_wild_card(const std::string & wildCard)
	{
		return wildCard.find_first_of("?*") != std::string::npos;
	}

	std::string wildcard_to_regex(const std::string & wildcard)
	{
		using namespace boost;
		std::string wild = escape_regex(wildcard);
		// "\*" -> .*
		replace_all_regex(wild, regex("\\\\\\*"), std::string(".*"));
		// "\?" -> .
		replace_all_regex(wild, regex("\\\\\\?"), std::string("."));
		return wild;
	}
}
