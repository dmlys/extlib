#pragma once

#include <algorithm>
#include <ext/algorithm/binary_find.hpp>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class ForwardIterator, class T>
	struct binary_find_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;
		const T & val;

		binary_find_visitor(ForwardIterator first, ForwardIterator last, const T & val)
			: first(first), last(last), val(val) {}
		
		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return ext::binary_find(first, last, val, pred);
		}
	};

	template <class ForwardIterator, class Type, class... VariantTypes>
	inline ForwardIterator binary_find(ForwardIterator first, ForwardIterator last,
	                                   const Type & val, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			binary_find_visitor<ForwardIterator, Type> {first, last, val},
			pred);
	}

	template <class ForwardIterator, class Type, class Pred>
	inline ForwardIterator binary_find(ForwardIterator first, ForwardIterator last,
		                               const Type & val, Pred pred)
	{
		return ext::binary_find(first, last, val, pred);
	}

	/// range overloads
	template <class ForwardRange, class Type, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		binary_find(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::
			binary_find(boost::begin(rng), boost::end(rng), val, pred);
	}

	template <class ForwardRange, class Type, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		binary_find(ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::
			binary_find(boost::begin(rng), boost::end(rng), val, pred);
	}

	template <boost::range_return_value re, class ForwardRange, class Type, class Pred>
	inline typename boost::range_return<const ForwardRange, re>::type
		binary_find(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return boost::range_return<const ForwardRange, re>::pack(
			varalgo::binary_find(rng, val, pred),
			rng);
	}

	template <boost::range_return_value re, class ForwardRange, class Type, class Pred>
	inline typename boost::range_return<ForwardRange, re>::type
		binary_find(ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return boost::range_return<ForwardRange, re>::pack(
			varalgo::binary_find(rng, val, pred),
			rng);
	}
}}
