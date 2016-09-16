#pragma once
#include <string> // for std::char_traits
#include <ext/strings/basic_string_facade.hpp>
#include <ext/strings/basic_string_facade_integration.hpp>
#include <ext/strings/cow_string_body.hpp>

namespace ext
{
	typedef ext::basic_string_facade<
		ext::cow_string_body, std::char_traits<char>
	> cow_string;
}
