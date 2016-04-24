#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo {

	template <class InputIterator1, class InputIterator2, class OutputIterator>
	struct merge_visitor : boost::static_visitor<OutputIterator>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;
		OutputIterator d_first;

		merge_visitor(InputIterator1 first1, InputIterator1 last1,
		              InputIterator2 first2, InputIterator2 last2,
		              OutputIterator d_first)
			: first1(first1), last1(last1), first2(first2), last2(last2), d_first(d_first) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred) const
		{
			return std::merge(first1, last1, first2, last2, d_first);
		}
	};


	template <class InputIterator1, class InputIterator2, class OutputIterator, class... VariantTypes>
	inline
	OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
	                     InputIterator2 first2, InputIterator2 last2,
	                     OutputIterator d_first, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			merge_visitor<InputIterator1, InputIterator2, OutputIterator> {first1, last1, first2, last2, d_first},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class OutputIterator, class Pred>
	inline
	OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
	                     InputIterator2 first2, InputIterator2 last2,
	                     OutputIterator d_first, Pred pred)
	{
		return std::merge(first1, last1, first2, last2, d_first, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class OutputIterator, class Pred>
	inline
	OutputIterator merge(const SinglePassRange1 & rng1, const SinglePassRange2 & rng2,
	                     OutputIterator d_first, const Pred & pred)
	{
		return varalgo::
			merge(boost::begin(rng1), boost::end(rng1),
		          boost::begin(rng2), boost::end(rng2),
		          d_first, pred);
	}
}}