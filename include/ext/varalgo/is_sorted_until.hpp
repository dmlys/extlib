#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class ForwardIterator>
	struct is_sorted_until_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;
		is_sorted_until_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::is_sorted_until(first, last, pred);
		}
	};

	template <class ForwardIterator, class... VariantTypes>
	inline ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last,
	                                       const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(is_sorted_until_visitor<ForwardIterator> {first, last}, pred);
	}

	template <class ForwardIterator, class Pred>
	inline ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last, Pred pred)
	{
		return std::is_sorted_until(first, last, pred);
	}

	template <class ForwardRange, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		is_sorted_until(const ForwardRange & rng, const Pred & pred)
	{
		return varalgo::is_sorted_until(boost::begin(rng), boost::end(rng), pred);
	}

	template <class ForwardRange, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		is_sorted_until(ForwardRange & rng, const Pred & pred)
	{
		return varalgo::is_sorted_until(boost::begin(rng), boost::end(rng), pred);
	}
}}