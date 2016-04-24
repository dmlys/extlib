#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/algorithm/cxx14/mismatch.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator1, class InputIterator2>
	struct mismatch_visitor_full :
		boost::static_visitor<std::pair<InputIterator1, InputIterator2>>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;

		mismatch_visitor_full(InputIterator1 first1, InputIterator1 last1,
		                      InputIterator2 first2, InputIterator2 last2)
			: first1(first1), last1(last1), first2(first2), last2(last2) {}

		template <class Pred>
		inline std::pair<InputIterator1, InputIterator2> operator()(Pred pred) const
		{
			return boost::algorithm::mismatch(first1, last1, first2, last2, pred);
		}
	};

	template <class InputIterator1, class InputIterator2>
	struct mismatch_visitor_half :
		boost::static_visitor<std::pair<InputIterator1, InputIterator2>>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2;

		mismatch_visitor_half(InputIterator1 first1, InputIterator1 last1,
		                      InputIterator2 first2)
			: first1(first1), last1(last1), first2(first2) {}

		template <class Pred>
		inline std::pair<InputIterator1, InputIterator2> operator()(Pred pred) const
		{
			return std::mismatch(first1, last1, first2, pred);
		}
	};

	template <class InputIterator1, class InputIterator2, class... VaraintTypes>
	inline std::pair<InputIterator1, InputIterator2>
		mismatch(InputIterator1 first1, InputIterator1 last1,
		         InputIterator2 first2, InputIterator2 last2,
		         const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			mismatch_visitor_full<InputIterator1, InputIterator2> {first1, last1, first2, last2},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class... VaraintTypes>
	inline std::pair<InputIterator1, InputIterator2>
		mismatch(InputIterator1 first1, InputIterator1 last1,
		         InputIterator2 first2,
		         const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			mismatch_visitor_half<InputIterator1, InputIterator2> {first1, last1, first2},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class Pred>
	inline std::pair<InputIterator1, InputIterator2>
		mismatch(InputIterator1 first1, InputIterator1 last1,
		         InputIterator2 first2, InputIterator2 last2,
		         Pred pred)
	{
		return boost::algorithm::mismatch(first1, last1, first2, last2, pred);
	}

	template <class InputIterator1, class InputIterator2, class Pred>
	inline std::pair<InputIterator1, InputIterator2>
		mismatch(InputIterator1 first1, InputIterator1 last1,
		         InputIterator2 first2,
		         Pred pred)
	{
		return std::mismatch(first1, last1, first2, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline std::pair<
		typename boost::range_iterator<const SinglePassRange1>::type,
		typename boost::range_iterator<const SinglePassRange2>::type
	>
		mismatch(const SinglePassRange1 & rng1, const SinglePassRange2 & rng2,
		         const Pred & pred)
	{
		return varalgo::
			mismatch(boost::begin(rng1), boost::end(rng1),
		             boost::begin(rng2), boost::end(rng2),
		             pred);
	}

	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline std::pair<
		typename boost::range_iterator<const SinglePassRange1>::type,
		typename boost::range_iterator<SinglePassRange2>::type
	>
		mismatch(const SinglePassRange1 & rng1, SinglePassRange2 & rng2,
		         const Pred & pred)
	{
		return varalgo::
			mismatch(boost::begin(rng1), boost::end(rng1),
		             boost::begin(rng2), boost::end(rng2),
		             pred);
	}

	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline std::pair<
		typename boost::range_iterator<SinglePassRange1>::type,
		typename boost::range_iterator<const SinglePassRange2>::type
	>
		mismatch(SinglePassRange1 & rng1, const SinglePassRange2 & rng2,
		         const Pred & pred)
	{
		return varalgo::
			mismatch(boost::begin(rng1), boost::end(rng1),
		             boost::begin(rng2), boost::end(rng2),
		             pred);
	}

	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline std::pair<
		typename boost::range_iterator<SinglePassRange1>::type,
		typename boost::range_iterator<SinglePassRange2>::type
	>
		mismatch(SinglePassRange1 & rng1, SinglePassRange2 & rng2,
		         const Pred & pred)
	{
		return varalgo::
			mismatch(boost::begin(rng1), boost::end(rng1),
		             boost::begin(rng2), boost::end(rng2),
		             pred);
	}
}}