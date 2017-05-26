#pragma once
#include <ext/type_traits.hpp>
#include <ext/range/range_traits.hpp>

namespace ext
{

	template <class Type>
	struct is_decayed_type_charlike :
		std::integral_constant
		<
			bool,
			std::is_same<Type, char>::value
		 || std::is_same<Type, wchar_t>::value
		 || std::is_same<Type, char16_t>::value
		 || std::is_same<Type, char32_t>::value
		>
	{ };

	template <class Type>
	struct is_char_type :
		is_decayed_type_charlike<std::decay_t<Type>> {};


	template <class Type>
	struct is_string : is_char_type<ext::range_value_t<Type>> {};

	template <class CharType>
	struct is_string<CharType *> : is_char_type<CharType> {};

	template <class CharType>
	struct is_string<CharType[]> : is_char_type<CharType> {};

	template <class CharType, std::size_t N>
	struct is_string<CharType[N]> : is_char_type<CharType> {};

	// range detection is done via boost::begin, boost::end,
	// and those work with std::pair, pair of anything is not a string.
	// sort of protection from std::pair<char *, char *>, etc
	template <class Type>
	struct is_string<std::pair<Type, Type>> : std::false_type {};

	template <class Type>
	struct is_string_range :
		ext::is_string<
			std::decay_t<ext::range_value_t<Type>>
		> {};

	template <class Type>
	struct is_string_or_string_range :
		std::integral_constant<bool,
			ext::is_string<Type>::value || ext::is_string_range<Type>::value
		>
	{ };
}
