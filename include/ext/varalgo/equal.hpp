#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/algorithm/cxx14/equal.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator1, class InputIterator2>
	struct equal_visitor_full :
		boost::static_visitor<bool>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;

		equal_visitor_full(InputIterator1 first1, InputIterator1 last1,
		                   InputIterator2 first2, InputIterator2 last2)
			: first1(first1), last1(last1), first2(first2), last2(last2) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return boost::algorithm::equal(first1, last1, first2, last2, pred);
		}
	};

	template <class InputIterator1, class InputIterator2>
	struct equal_visitor_half :
		boost::static_visitor<bool>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2;

		equal_visitor_half(InputIterator1 first1, InputIterator1 last1,
		                   InputIterator2 first2)
			: first1(first1), last1(last1), first2(first2) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::equal(first1, last1, first2, pred);
		}
	};

	template <class InputIterator1, class InputIterator2, class... VaraintTypes>
	inline bool
		equal(InputIterator1 first1, InputIterator1 last1,
		      InputIterator2 first2, InputIterator2 last2,
		      const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			equal_visitor_full<InputIterator1, InputIterator2> {first1, last1, first2, last2},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class... VaraintTypes>
	inline bool
		equal(InputIterator1 first1, InputIterator1 last1,
		      InputIterator2 first2,
		      const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			equal_visitor_half<InputIterator1, InputIterator2> {first1, last1, first2},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class Pred>
	inline bool
		equal(InputIterator1 first1, InputIterator1 last1,
		      InputIterator2 first2, InputIterator2 last2,
		      Pred pred)
	{
		return boost::algorithm::equal(first1, last1, first2, last2, pred);
	}

	template <class InputIterator1, class InputIterator2, class Pred>
	inline bool
		equal(InputIterator1 first1, InputIterator1 last1,
		      InputIterator2 first2,
		      Pred pred)
	{
		return std::equal(first1, last1, first2, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline bool
		equal(const SinglePassRange1 & rng1, const SinglePassRange2 & rng2,
		      const Pred & pred)
	{
		return varalgo::
			equal(boost::begin(rng1), boost::end(rng1),
		          boost::begin(rng2), boost::end(rng2),
		          pred);
	}
}}