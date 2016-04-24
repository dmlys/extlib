#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class BidirectionalIterator>
	struct stable_partition_visitor : boost::static_visitor<BidirectionalIterator>
	{
		BidirectionalIterator first, last;
		stable_partition_visitor(BidirectionalIterator first, BidirectionalIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline BidirectionalIterator operator()(Pred pred) const
		{
			return std::stable_partition(first, last, pred);
		}
	};

	template <class BidirectionalIterator, class... VariantTypes>
	inline BidirectionalIterator stable_partition(BidirectionalIterator first, BidirectionalIterator last,
	                                       const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(stable_partition_visitor<BidirectionalIterator> {first, last}, pred);
	}

	template <class BidirectionalIterator, class Pred>
	inline BidirectionalIterator stable_partition(BidirectionalIterator first, BidirectionalIterator last, Pred pred)
	{
		return std::stable_partition(first, last, pred);
	}

	/// range overload
	template <class BidirectionalRange, class Pred>
	inline typename boost::range_const_iterator<BidirectionalRange>::type
		stable_partition(const BidirectionalRange & rng, const Pred & pred)
	{
		return varalgo::stable_partition(boost::begin(rng), boost::end(rng), pred);
	}

	template <class BidirectionalRange, class Pred>
	inline typename boost::range_iterator<BidirectionalRange>::type
		stable_partition(BidirectionalRange & rng, const Pred & pred)
	{
		return varalgo::stable_partition(boost::begin(rng), boost::end(rng), pred);
	}

	/// range overload
	template <boost::range_return_value re, class BidirectionalRange, class Pred>
	inline typename boost::range_return<const BidirectionalRange, re>::type
		stable_partition(const BidirectionalRange & rng, const Pred & pred)
	{
		return boost::range_return<const BidirectionalRange, re>::pack(
			varalgo::stable_partition(rng, pred),
			rng);
	}

	template <boost::range_return_value re, class BidirectionalRange, class Pred>
	inline typename boost::range_return<BidirectionalRange, re>::type
		stable_partition(BidirectionalRange & rng, const Pred & pred)
	{
		return boost::range_return<BidirectionalRange, re>::pack(
			varalgo::stable_partition(rng, pred),
			rng);
	}
}}