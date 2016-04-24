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
	struct stable_sort_visitor : boost::static_visitor<void>
	{
		RandomAccessIterator first, last;

		stable_sort_visitor(RandomAccessIterator first, RandomAccessIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::stable_sort(first, last, pred);
		}
	};

	template <class RandomAccessIterator, class... VariantTypes>
	inline void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	                        const boost::variant<VariantTypes...> &  pred)
	{
		boost::apply_visitor(stable_sort_visitor<RandomAccessIterator> {first, last}, pred);
	}

	template <class RandomAccessIterator, class Pred>
	inline void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	                        Pred pred)
	{
		std::stable_sort(first, last, pred);
	}

	/// range overloads
	template <class RandomAccessRange, class Pred>
	inline const RandomAccessRange & stable_sort(const RandomAccessRange & rng, const Pred & pred)
	{
		varalgo::stable_sort(boost::begin(rng), boost::end(rng), pred);
		return rng;
	}

	template <class RandomAccessRange, class Pred>
	inline RandomAccessRange & stable_sort(RandomAccessRange & rng, const Pred & pred)
	{
		varalgo::stable_sort(boost::begin(rng), boost::end(rng), pred);
		return rng;
	}
}}
