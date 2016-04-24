#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class InputIterator, class RandomAccessIterator>
	struct partial_sort_copy_visitor : boost::static_visitor<RandomAccessIterator>
	{
		InputIterator first, last;
		RandomAccessIterator d_first, d_last;

		partial_sort_copy_visitor(InputIterator first, InputIterator last, RandomAccessIterator d_first, RandomAccessIterator d_last)
			: first(first), last(last), d_first(d_first), d_last(d_last) {}

		template <class Pred>
		inline RandomAccessIterator operator()(Pred pred) const
		{
			return std::partial_sort_copy(first, last, d_first, d_last, pred);
		}
	};

	template <class InputIterator, class RandomAccessIterator, class... VariantTypes>
	inline RandomAccessIterator partial_sort_copy(InputIterator first, InputIterator last,
	                                              RandomAccessIterator d_first, RandomAccessIterator d_last,
	                                              const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			partial_sort_copy_visitor<InputIterator, RandomAccessIterator> {first, last, d_first, d_last},
			pred);
	}

	template <class InputIterator, class RandomAccessIterator, class Pred>
	inline RandomAccessIterator partial_sort_copy(InputIterator first, InputIterator last,
	                                              RandomAccessIterator d_first, RandomAccessIterator d_last,
	                                              Pred pred)
	{
		return std::partial_sort_copy(first, last, d_first, d_last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class RandomAccessRange, class Pred>
	inline typename boost::range_const_iterator<SinglePassRange>::type
		partial_sort_copy(const SinglePassRange & input, const RandomAccessRange & output, const Pred & pred)
	{
		return varalgo::
			partial_sort_copy(boost::begin(input), boost::end(input),
			                  boost::begin(output), boost::end(output),
			                  pred);
	}

	template <class SinglePassRange, class RandomAccessRange, class Pred>
	inline typename boost::range_iterator<SinglePassRange>::type
		partial_sort_copy(const SinglePassRange & input, RandomAccessRange & output, const Pred & pred)
	{
		return varalgo::
			partial_sort_copy(boost::begin(input), boost::end(input),
			                  boost::begin(output), boost::end(output),
			                  pred);
	}
}}
