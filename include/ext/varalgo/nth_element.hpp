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
	struct nth_element_visitor : boost::static_visitor<void>
	{
		RandomAccessIterator first, nth, last;

		nth_element_visitor(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last)
			: first(first), nth(nth), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::nth_element(first, nth, last, pred);
		}
	};

	template <class RandomAccessIterator, class... VariantTypes>
	inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last,
	                        const boost::variant<VariantTypes...> & pred)
	{
		boost::apply_visitor(
			nth_element_visitor<RandomAccessIterator> {first, nth, last},
			pred);
	}

	template <class RandomAccessIterator, class Pred>
	inline void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last,
	                        Pred pred)
	{
		std::nth_element(first, nth, last, pred);
	}

	/// range overloads
	template <class RandomAccessRange, class Pred>
	inline const RandomAccessRange &
		nth_element(const RandomAccessRange & rng,
		            typename boost::range_iterator<const RandomAccessRange>::type nth,
		            const Pred & pred)
	{
		varalgo::nth_element(boost::begin(rng), nth, boost::end(rng), pred);
		return rng;
	}

	template <class RandomAccessRange, class Pred>
	inline RandomAccessRange &
		nth_element(RandomAccessRange & rng,
		            typename boost::range_iterator<RandomAccessRange>::type nth,
		            const Pred & pred)
	{
		varalgo::nth_element(boost::begin(rng), nth, boost::end(rng), pred);
		return rng;
	}
}}

