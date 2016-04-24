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
		const boost::regex escapeSymbols("[.\\[\\]{}()\\\\*+?|^$]");
		// заменяем на \<найденное>
		// $& - full expr match placeholder
		const std::string format = "\\\\$&";
	}

	std::string escape_regex(std::string const & regx)
	{
		return boost::replace_all_regex_copy(regx, escapeSymbols, format);
	}

	bool is_wild_card(std::string const & wildCard)
	{
		return wildCard.find_first_of("?*") != std::string::npos;
	}

	std::string wildcard_to_regex(std::string const & wildcard)
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