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
	struct sort_visitor : boost::static_visitor<void>
	{
		RandomAccessIterator first, last;
		sort_visitor(RandomAccessIterator first, RandomAccessIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			return std::sort(first, last, pred);
		}
	};

	template <class RandomAccessIterator, class... VariantTypes>
	inline void sort(RandomAccessIterator first, RandomAccessIterator last,
	                 const boost::variant<VariantTypes...> & pred)
	{
		boost::apply_visitor(sort_visitor<RandomAccessIterator> {first, last}, pred);
	}

	template <class RandomAccessIterator, class Pred>
	inline void sort(RandomAccessIterator first, RandomAccessIterator last, Pred pred)
	{
		std::sort(first, last, pred);
	}

	/// range overloads
	template <class RandomAccessRange, class Pred>
	inline const RandomAccessRange & sort(const RandomAccessRange & rng, const Pred & pred)
	{
		varalgo::sort(boost::begin(rng), boost::end(rng), pred);
		return rng;
	}

	template <class RandomAccessRange, class Pred>
	inline RandomAccessRange & sort(RandomAccessRange & rng, const Pred & pred)
	{
		varalgo::sort(boost::begin(rng), boost::end(rng), pred);
		return rng;
	}
}}
