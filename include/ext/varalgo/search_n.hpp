#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class ForwardIterator1, class Integer, class Type>
	struct search_n_visitor :
		boost::static_visitor<ForwardIterator1>
	{
		ForwardIterator1 first1, last1;
		Integer n;
		const Type & val;

		search_n_visitor(ForwardIterator1 first1, ForwardIterator1 last1,
		                 Integer n, const Type & val)
			: first1(first1), last1(last1), n(n), val(val) {}

		template <class Pred>
		inline ForwardIterator1 operator()(Pred pred) const
		{
			return std::search_n(first1, last1, n, val);
		}
	};

	template <class ForwardIterator1, class Integer, class Type, class... VaraintTypes>
	inline ForwardIterator1
		search_n(ForwardIterator1 first1, ForwardIterator1 last1,
		         Integer n, const Type & val,
		         const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			search_n_visitor<ForwardIterator1, Integer, Type> {first1, last1, n, val},
			pred);
	}

	template <class ForwardIterator1, class Integer, class Type, class Pred>
	inline ForwardIterator1
		search_n(ForwardIterator1 first1, ForwardIterator1 last1,
		         Integer n, const Type & val,
		         Pred pred)
	{
		return std::search_n(first1, last1, n, val, pred);
	}

	/// range overloads
	template <class ForwardRange1, class Integer, class Type, class Pred>
	inline typename boost::range_iterator<const ForwardRange1>::type
		search_n(const ForwardRange1 & rng1,
		         Integer n, const Type & val,
		         const Pred & pred)
	{
		return varalgo::
			search_n(boost::begin(rng1), boost::end(rng1),
			         n, val, pred);
	}

	template <class ForwardRange1, class Integer, class Type, class Pred>
	inline typename boost::range_iterator<ForwardRange1>::type
		search_n(ForwardRange1 & rng1,
		         Integer n, const Type & val,
		         const Pred & pred)
	{
		return varalgo::
			search_n(boost::begin(rng1), boost::end(rng1),
			         n, val, pred);
	}

	template <boost::range_return_value re, class ForwardRange1, class Integer, class Type, class Pred>
	inline typename boost::range_return<const ForwardRange1, re>::type
		search_n(const ForwardRange1 & rng1,
		         Integer n, const Type & val,
		         const Pred & pred)
	{
		return boost::range_return<ForwardRange1, re>::pack(
			varalgo::search_n(rng1, n, val, pred),
		    rng1);
	}

	template <boost::range_return_value re, class ForwardRange1, class Integer, class Type, class Pred>
	inline typename boost::range_return<ForwardRange1, re>::type
		search_n(ForwardRange1 & rng1,
		         Integer n, const Type & val,
		         const Pred & pred)
	{
		return boost::range_return<ForwardRange1, re>::pack(
		    varalgo::search_n(rng1, n, val, pred),
		    rng1);
	}
}}