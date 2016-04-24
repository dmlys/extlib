#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator>
	struct find_if_not_visitor :
		boost::static_visitor<InputIterator>
	{
		InputIterator first, last;

		find_if_not_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline InputIterator operator()(Pred pred) const
		{
			return std::find_if_not(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline InputIterator
		find_if_not(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			find_if_not_visitor<InputIterator> {first, last},
			pred);
	}
	
	template <class InputIterator, class Pred>
	inline InputIterator
		find_if_not(InputIterator first, InputIterator last, Pred pred)
	{
		return std::find_if_not(first, last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class Pred>
	inline typename boost::range_iterator<const SinglePassRange>::type
		find_if_not(const SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::find_if_not(boost::begin(rng), boost::end(rng), pred);
	}

	template <class SinglePassRange, class Pred>
	inline typename boost::range_iterator<SinglePassRange>::type
		find_if_not(SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::find_if_not(boost::begin(rng), boost::end(rng), pred);
	}

	template <boost::range_return_value re, class SinglePassRange, class Pred>
	inline typename boost::range_return<const SinglePassRange, re>::type
		find_if_not(const SinglePassRange & rng, const Pred & pred)
	{
		return boost::range_return<const SinglePassRange, re>::pack(
			varalgo::find_if_not(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	template <boost::range_return_value re, class SinglePassRange, class Pred>
	inline typename boost::range_return<SinglePassRange, re>::type
		find_if_not(SinglePassRange & rng, const Pred & pred)
	{
		return boost::range_return<SinglePassRange, re>::pack(
			varalgo::find_if_not(boost::begin(rng), boost::end(rng), pred),
			rng);
	}
}}