#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator1, class ForwardIterator2>
	struct find_first_of_visitor :
		boost::static_visitor<InputIterator1>
	{
		InputIterator1 first1, last1;
		ForwardIterator2 first2, last2;

		find_first_of_visitor(InputIterator1 first1, InputIterator1 last1,
		                      ForwardIterator2 first2, ForwardIterator2 last2)
			: first1(first1), last1(last1), first2(first2), last2(last2) {}

		template <class Pred>
		inline InputIterator1 operator()(Pred pred) const
		{
			return std::find_first_of(first1, last1, first2, last2, pred);
		}
	};

	template <class InputIterator1, class ForwardIterator2, class... VaraintTypes>
	inline InputIterator1
		find_first_of(InputIterator1 first1, InputIterator1 last1,
		              ForwardIterator2 first2, ForwardIterator2 last2,
		              const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			find_first_of_visitor<InputIterator1, ForwardIterator2> {first1, last1, first2, last2},
			pred);
	}

	template <class InputIterator1, class ForwardIterator2, class Pred>
	inline InputIterator1
		find_first_of(InputIterator1 first1, InputIterator1 last1,
		              ForwardIterator2 first2, ForwardIterator2 last2,
		              Pred pred)
	{
		return std::find_first_of(first1, last1, first2, last2, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class ForwardRange2, class Pred>
	inline typename boost::range_iterator<const SinglePassRange1>::type
		find_first_of(const SinglePassRange1 & rng1, const ForwardRange2 & rng2,
		              const Pred & pred)
	{
		return varalgo::
			find_first_of(boost::begin(rng1), boost::end(rng1),
		                  boost::begin(rng2), boost::end(rng2),
		                  pred);
	}

	template <class SinglePassRange1, class ForwardRange2, class Pred>
	inline typename boost::range_iterator<SinglePassRange1>::type
		find_first_of(SinglePassRange1 & rng1, const ForwardRange2 & rng2,
		              const Pred & pred)
	{
		return varalgo::
			find_first_of(boost::begin(rng1), boost::end(rng1),
		                  boost::begin(rng2), boost::end(rng2),
		                  pred);
	}

	template <boost::range_return_value re, class SinglePassRange1, class ForwardRange2, class Pred>
	inline typename boost::range_return<const SinglePassRange1, re>::type
		find_first_of(const SinglePassRange1 & rng1, const ForwardRange2 & rng2,
		              const Pred & pred)
	{
		return boost::range_return<SinglePassRange1, re>::pack(
			varalgo::find_first_of(rng1, rng2, pred),
		    rng1);
	}

	template <boost::range_return_value re, class SinglePassRange1, class ForwardRange2, class Pred>
	inline typename boost::range_return<SinglePassRange1, re>::type
		find_first_of(SinglePassRange1 & rng1, const ForwardRange2 & rng2,
		         const Pred & pred)
	{
		return boost::range_return<SinglePassRange1, re>::pack(
			varalgo::find_first_of(rng1, rng2, pred),
		    rng1);
	}
}}