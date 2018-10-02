#pragma once
#ifndef REGEX_UTILS_H_
#define REGEX_UTILS_H_

#include <string>

namespace ext
{
	/// эскеипит спец символы регекс выражения.
	/// .[{}()\*+?|^$
	std::string escape_regex(const std::string & regx);

	///проверяет сожержит ли строка символы wildCard
	bool is_wild_card(const std::string & wildCard);
	/// Преобразует выражение маски (DUMMY.*) в соотвествующее ему регекс выражение
	std::string wildcard_to_regex(const std::string & wildcard);
}


#endif
