#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class ForwardIterator, class T>
	struct equal_range_visitor : boost::static_visitor<std::pair<ForwardIterator, ForwardIterator>>
	{
		ForwardIterator first, last;
		const T & val;

		equal_range_visitor(ForwardIterator first, ForwardIterator last, const T & val)
			: first(first), last(last), val(val) {}

		template <class Pred>
		inline std::pair<ForwardIterator, ForwardIterator> operator()(Pred pred) const
		{
			return std::equal_range(first, last, val, pred);
		}
	};

	template <class ForwardIterator, class Type, class... VariantTypes>
	inline std::pair<ForwardIterator, ForwardIterator>
	equal_range(ForwardIterator first, ForwardIterator last,
	            const Type & val, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			equal_range_visitor<ForwardIterator, Type> {first, last, val},
			pred);
	}

	template <class ForwardIterator, class Type, class Pred>
	inline std::pair<ForwardIterator, ForwardIterator>
	equal_range(ForwardIterator first, ForwardIterator last,
	            const Type & val, Pred pred)
	{
		return std::equal_range(first, last, val, pred);
	}

	/// range overloads
	template <class ForwardRange, class Type, class Pred>
	inline
	std::pair<
		typename boost::range_const_iterator<ForwardRange>::type,
		typename boost::range_const_iterator<ForwardRange>::type
	>
	equal_range(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::equal_range(boost::begin(rng), boost::end(rng), val, pred);
	}

	template <class ForwardRange, class Type, class Pred>
	inline
	std::pair<
		typename boost::range_iterator<ForwardRange>::type,
		typename boost::range_iterator<ForwardRange>::type
	>
	equal_range(ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::equal_range(boost::begin(rng), boost::end(rng), val, pred);
	}
}}

