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
	template <class ForwardIterator, class T>
	struct upper_bound_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;
		const T & val;

		upper_bound_visitor(ForwardIterator first, ForwardIterator last, const T & val)
			: first(first), last(last), val(val) {}
		
		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::upper_bound(first, last, val, pred);
		}
	};

	template <class ForwardIterator, class Type, class... VariantTypes>
	inline ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
	                                   const Type & val, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			upper_bound_visitor<ForwardIterator, Type> {first, last, val},
			pred);
	}

	template <class ForwardIterator, class Type, class Pred>
	inline ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
	                                   const Type & val, Pred pred)
	{
		return std::upper_bound(first, last, val, pred);
	}

	/// range overloads
	template <class ForwardRange, class Type, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		upper_bound(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::
			upper_bound(boost::begin(rng), boost::end(rng), val, pred);
	}

	template <class ForwardRange, class Type, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		upper_bound(ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::
			upper_bound(boost::begin(rng), boost::end(rng), val, pred);
	}

	/// range overloads
	template <boost::range_return_value re, class ForwardRange, class Type, class Pred>
	inline typename boost::range_return<const ForwardRange, re>::type
		upper_bound(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return boost::range_return<const ForwardRange, re>::pack(
			varalgo::upper_bound(rng, val, pred),
			rng);
	}

	template <boost::range_return_value re, class ForwardRange, class Type, class Pred>
	inline typename boost::range_return<ForwardRange, re>::type
		upper_bound(ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return boost::range_return<ForwardRange, re>::pack(
			varalgo::upper_bound(rng, val, pred),
			rng);
	}
}}
