#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class RandomAccessIterator>
	struct partial_sort_visitor : boost::static_visitor<void>
	{
		RandomAccessIterator first, middle, last;

		partial_sort_visitor(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
			: first(first), middle(middle), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::partial_sort(first, middle, last, pred);
		}
	};

	template <class RandomAccessIterator, class... VariantTypes>
	inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
	                         const boost::variant<VariantTypes...> & pred)
	{
		boost::apply_visitor(partial_sort_visitor<RandomAccessIterator> {first, middle, last}, pred);
	}

	template <class RandomAccessIterator, class Pred>
	inline void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
	                         Pred pred)
	{
		std::partial_sort(first, middle, last, pred);
	}

	/// range overloads
	template <class RandomAccessRange, class Pred>
	inline const RandomAccessRange &
		partial_sort(const RandomAccessRange & rng,
		             typename boost::range_iterator<const RandomAccessRange>::type middle,
		             const Pred & pred)
	{
		varalgo::partial_sort(boost::begin(rng), middle, boost::end(rng), pred);
		return rng;
	}
	
	template <class RandomAccessRange, class Pred>
	inline RandomAccessRange &
		partial_sort(RandomAccessRange & rng,
		             typename boost::range_iterator<RandomAccessRange>::type middle,
		             const Pred & pred)
	{
		varalgo::partial_sort(boost::begin(rng), middle, boost::end(rng), pred);
		return rng;
	}
}}
