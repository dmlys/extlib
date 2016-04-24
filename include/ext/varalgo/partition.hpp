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
	template <class ForwardIterator>
	struct partition_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;
		partition_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::partition(first, last, pred);
		}
	};

	template <class ForwardIterator, class... VariantTypes>
	inline ForwardIterator partition(ForwardIterator first, ForwardIterator last,
	                                       const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(partition_visitor<ForwardIterator> {first, last}, pred);
	}

	template <class ForwardIterator, class Pred>
	inline ForwardIterator partition(ForwardIterator first, ForwardIterator last, Pred pred)
	{
		return std::partition(first, last, pred);
	}

	/// range overload
	template <class ForwardRange, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		partition(const ForwardRange & rng, const Pred & pred)
	{
		return varalgo::partition(boost::begin(rng), boost::end(rng), pred);
	}

	template <class ForwardRange, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		partition(ForwardRange & rng, const Pred & pred)
	{
		return varalgo::partition(boost::begin(rng), boost::end(rng), pred);
	}

	/// range overload
	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<const ForwardRange, re>::type
		partition(const ForwardRange & rng, const Pred & pred)
	{
		return boost::range_return<const ForwardRange, re>::pack(
			varalgo::partition(rng, pred),
			rng);
	}

	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<ForwardRange, re>::type
		partition(ForwardRange & rng, const Pred & pred)
	{
		return boost::range_return<ForwardRange, re>::pack(
			varalgo::partition(rng, pred),
			rng);
	}
}}