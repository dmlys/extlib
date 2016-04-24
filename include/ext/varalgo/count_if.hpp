#pragma once

#include <algorithm>
#include <ext/algorithm/binary_find.hpp>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator>
	struct count_if_visitor :
		boost::static_visitor<typename boost::iterator_difference<InputIterator>::type>
	{
		InputIterator first, last;

		count_if_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline typename boost::iterator_difference<InputIterator>::type operator()(Pred pred) const
		{
			return std::count_if(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline typename boost::iterator_difference<InputIterator>::type
		count_if(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			count_if_visitor<InputIterator> {first, last},
			pred);
	}
	
	template <class InputIterator, class Pred>
	inline typename boost::iterator_difference<InputIterator>::type
		count_if(InputIterator first, InputIterator last, Pred pred)
	{
		return std::count_if(first, last, pred);
	}

	/// range overload
	template <class SinglePassRange, class Pred>
	inline typename boost::range_difference<SinglePassRange>::type
		count_if(const SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::count_if(boost::begin(rng), boost::end(rng), pred);
	}
}}