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



	namespace detail
	{
		template <class Type, bool = is_range<Type>::value>
		struct is_string : std::false_type {};

		template <class Range>
		struct is_string<Range, true> :
			ext::is_char_type<typename boost::range_value<Range>::type>
		{ };

		// range detection is done via boost::begin, boost::end, 
		// and those work with std::pair, pair of anything is not a string.
		// sort of protection from std::pair<char *, char *>, etc
		template <class Type>
		struct is_string<std::pair<Type, Type>, true> : std::false_type {};
	}

	template <class Type>
	struct is_string : detail::is_string<Type> {};



	namespace detail
	{
		template <class Type, bool = is_range<Type>::value>
		struct is_string_range : std::false_type {};

		template <class Range>
		struct is_string_range<Range, true> : 
			ext::is_string<std::decay_t<typename boost::range_value<Range>::type>>
		{};
	}

	template <class Type>
	struct is_string_range : detail::is_string_range<Type> {};

	template <class Type>
	struct is_string_or_string_range : 
		std::integral_constant<bool,
			ext::is_string<Type>::value || ext::is_string_range<Type>::value
		>
	{ };
}
