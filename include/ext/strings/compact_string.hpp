#pragma once
#include <string> // for std::char_traits
#include <ext/strings/basic_string_facade.hpp>
#include <ext/strings/basic_string_facade_integration.hpp>
#include <ext/strings/compact_string_body.hpp>

namespace ext
{
	typedef ext::basic_string_facade<
		ext::compact_string_base,
		std::char_traits<char>
	> compact_string;
}
