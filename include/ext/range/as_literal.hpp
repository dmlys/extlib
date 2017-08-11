#pragma once
// author: Dmitry Lysachenko
// date: Tuesday 07 February 2017
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt
//          

#include <ciso646>
#include <cassert>

#include <boost/static_assert.hpp>
#include <ext/range/range_traits.hpp>
#include <boost/range/as_literal.hpp> // needed for some boost::range char * integration

namespace ext
{
	/// same as boost::as_literal, but always returns iterator_range of pointers(boost::iterator_range<Type *>),
	/// thus working only with contiguous string ranges, string literals and std::string are those.

	template <class String>
	inline auto as_literal(String & str) ->
		std::enable_if_t<
			ext::is_contiguous_container<String>::value,
			boost::iterator_range<typename boost::range_value<String>::type *>
		>
	{
		auto * ptr = ext::data(str);
		return {ptr, ptr + boost::size(str)};
	}
	
	template <class String>
	inline auto as_literal(const String & str) ->
		std::enable_if_t<
			ext::is_contiguous_container<String>::value,
			boost::iterator_range<typename boost::range_value<String>::type const *>
		>
	{
		auto * ptr = ext::data(str);
		return {ptr, ptr + boost::size(str)};
	}

	template <class String>
	inline auto as_literal(String & str) ->
		std::enable_if_t<
			not ext::is_contiguous_container<String>::value,
			boost::iterator_range<typename boost::range_iterator<String>::type>
		>
	{
		return boost::make_iterator_range(str);
	}

	template <class String>
	inline auto as_literal(const String & str) ->
		std::enable_if_t<
			not ext::is_contiguous_container<String>::value,
			boost::iterator_range<typename boost::range_iterator<const String>::type>
		>
	{
		return boost::make_iterator_range(str);
	}

	
	template <class CharType, std::size_t N>
	inline auto as_literal(CharType (& str)[N]) ->
		boost::iterator_range<CharType *>
	{
		assert(str[N - 1] == 0); // null terminated string
		return {str, str + N - 1};
	}
	
	template <class CharType>
	inline auto as_literal(CharType *& str) ->
		boost::iterator_range<CharType *>
	{
		return {str, str + std::char_traits<std::decay_t<CharType>>::length(str)};
	}

	template <class CharType>
	inline auto as_literal(CharType * const & str) ->
		boost::iterator_range<CharType *>
	{
		return {str, str + std::char_traits<std::decay_t<CharType>>::length(str)};
	}
	
	template <class Range>
	inline auto as_array(Range & range) ->
		boost::iterator_range<typename boost::range_iterator<Range>::type>
	{
		return boost::make_iterator_range(range);
	}
}
