#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo {

	template <class BidirectionalIterator>
	struct inplace_merge_visitor : boost::static_visitor<void>
	{
		BidirectionalIterator first, middle, last;

		inplace_merge_visitor(BidirectionalIterator first, BidirectionalIterator middle, BidirectionalIterator last)
			: first(first), middle(middle), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::inplace_merge(first, middle, last, pred);
		}
	};

	template <class BidirectionalIterator, class... VariantTypes>
	inline
	void inplace_merge(BidirectionalIterator first, BidirectionalIterator middle, BidirectionalIterator last,
	                   const boost::variant<VariantTypes...> & pred)
	{
		boost::apply_visitor(
			inplace_merge_visitor<BidirectionalIterator> {first, middle, last},
			pred);
	}

	template <class BidirectionalIterator, class Pred>
	inline
	void inplace_merge(BidirectionalIterator first, BidirectionalIterator middle, BidirectionalIterator last,
	                   Pred pred)
	{
		std::inplace_merge(first, middle, last, pred);
	}

	template <class BidirectionalRange, class Pred>
	inline
	const BidirectionalRange & inplace_merge(const BidirectionalRange & rng,
	                                         typename boost::range_const_iterator<BidirectionalRange>::type middle,
	                                         const Pred & pred)
	{
		varalgo::inplace_merge(boost::begin(rng), middle, boost::end(rng), pred);
		return rng;
	}

	template <class BidirectionalRange, class Pred>
	inline
	BidirectionalRange & inplace_merge(BidirectionalRange & rng,
	                                   typename boost::range_iterator<BidirectionalRange>::type middle,
	                                   const Pred & pred)
	{
		varalgo::inplace_merge(boost::begin(rng), middle, boost::end(rng), pred);
		return rng;
	}
}}