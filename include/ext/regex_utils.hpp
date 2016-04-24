#pragma once
#ifndef REGEX_UTILS_H_
#define REGEX_UTILS_H_

#include <string>

namespace ext
{
	/// эскеипит спец символы регекс выражения.
	/// .[{}()\*+?|^$
	std::string escape_regex(std::string const & regx);

	///проверяет сожержит ли строка символы wildCard
	bool is_wild_card(std::string const & wildCard);
	/// Преобразует выражение маски (DUMMY.*) в соотвествующее ему регекс выражение
	std::string wildcard_to_regex(std::string const & wildcard);
}


#endif