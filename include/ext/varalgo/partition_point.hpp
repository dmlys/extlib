#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator>
	struct partition_point_visitor :
		boost::static_visitor<InputIterator>
	{
		InputIterator first, last;

		partition_point_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline InputIterator operator()(Pred pred) const
		{
			return std::partition_point(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline InputIterator
		partition_point(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			partition_point_visitor<InputIterator> {first, last},
			pred);
	}
	
	template <class InputIterator, class Pred>
	inline InputIterator
		partition_point(InputIterator first, InputIterator last, Pred pred)
	{
		return std::partition_point(first, last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class Pred>
	inline typename boost::range_iterator<const SinglePassRange>::type
		partition_point(const SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::partition_point(boost::begin(rng), boost::end(rng), pred);
	}

	template <class SinglePassRange, class Pred>
	inline typename boost::range_iterator<SinglePassRange>::type
		partition_point(SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::partition_point(boost::begin(rng), boost::end(rng), pred);
	}

	template <boost::range_return_value re, class SinglePassRange, class Pred>
	inline typename boost::range_return<const SinglePassRange, re>::type
		partition_point(const SinglePassRange & rng, const Pred & pred)
	{
		return boost::range_return<const SinglePassRange, re>::pack(
			varalgo::partition_point(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	template <boost::range_return_value re, class SinglePassRange, class Pred>
	inline typename boost::range_return<SinglePassRange, re>::type
		partition_point(SinglePassRange & rng, const Pred & pred)
	{
		return boost::range_return<SinglePassRange, re>::pack(
			varalgo::partition_point(boost::begin(rng), boost::end(rng), pred),
			rng);
	}
}}