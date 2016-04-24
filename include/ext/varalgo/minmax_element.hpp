#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo {

	template <class ForwardIterator>
	struct min_element_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;

		min_element_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::min_element(first, last, pred);
		}
	};

	template <class ForwardIterator>
	struct max_element_visitor : boost::static_visitor<ForwardIterator>
	{
		ForwardIterator first, last;

		max_element_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::max_element(first, last, pred);
		}
	};

	template <class ForwardIterator>
	struct minmax_element_visitor : boost::static_visitor<std::pair<ForwardIterator,ForwardIterator>>
	{
		ForwardIterator first, last;

		minmax_element_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline ForwardIterator operator()(Pred pred) const
		{
			return std::minmax_element(first, last, pred);
		}
	};

	/************************************************************************/
	/*                     min_element                                      */
	/************************************************************************/
	template <class ForwardIterator, class... VariantTypes>
	inline ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
	                                   const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			min_element_visitor<ForwardIterator> {first, last},
			pred);
	}

	template <class ForwardIterator, class Pred>
	inline ForwardIterator min_element(ForwardIterator first, ForwardIterator last, Pred pred)
	{
		return std::min_element(first, last, pred);
	}

	/// range overloads
	template <class ForwardRange, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		min_element(const ForwardRange & rng, Pred pred)
	{
		return min_element(boost::begin(rng), boost::end(rng), pred);
	}

	template <class ForwardRange, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		min_element(ForwardRange & rng, Pred pred)
	{
		return min_element(boost::begin(rng), boost::end(rng), pred);
	}

	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<const ForwardRange, re>::type
		min_element(const ForwardRange & rng, Pred pred)
	{
		return boost::range_return<const ForwardRange, re>::pack(
			min_element(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<ForwardRange, re>::type
		min_element(ForwardRange & rng, Pred pred)
	{
		return boost::range_return<ForwardRange, re>::pack(
			min_element(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	/************************************************************************/
	/*                max_element                                           */
	/************************************************************************/
	template <class ForwardIterator, class... VariantTypes>
	inline ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
	                                   const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			max_element_visitor<ForwardIterator> {first, last},
			pred);
	}

	template <class ForwardIterator, class Pred>
	inline ForwardIterator max_element(ForwardIterator first, ForwardIterator last, Pred pred)
	{
		return std::max_element(first, last, pred);
	}

	/// range overloads
	template <class ForwardRange, class Pred>
	inline typename boost::range_const_iterator<ForwardRange>::type
		max_element(const ForwardRange & rng, Pred pred)
	{
		return max_element(boost::begin(rng), boost::end(rng), pred);
	}

	template <class ForwardRange, class Pred>
	inline typename boost::range_iterator<ForwardRange>::type
		max_element(ForwardRange & rng, Pred pred)
	{
		return max_element(boost::begin(rng), boost::end(rng), pred);
	}

	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<const ForwardRange, re>::type
		max_element(const ForwardRange & rng, Pred pred)
	{
		return boost::range_return<const ForwardRange, re>::pack(
			max_element(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	template <boost::range_return_value re, class ForwardRange, class Pred>
	inline typename boost::range_return<ForwardRange, re>::type
		max_element(ForwardRange & rng, Pred pred)
	{
		return boost::range_return<ForwardRange, re>::pack(
			max_element(boost::begin(rng), boost::end(rng), pred),
			rng);
	}

	/************************************************************************/
	/*                minmax_element                                        */
	/************************************************************************/
	template <class ForwardIterator, class... VariantTypes>
	inline std::pair<ForwardIterator, ForwardIterator>
		minmax_element(ForwardIterator first, ForwardIterator last,
		               const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			minmax_element_visitor<ForwardIterator> {first, last},
			pred);
	}

	template <class ForwardIterator, class Pred>
	inline std::pair<ForwardIterator, ForwardIterator>
		minmax_element(ForwardIterator first, ForwardIterator last, Pred pred)
	{
		return std::minmax_element(first, last, pred);
	}

	/// range overloads
	template <class ForwardRange, class Pred>
	inline std::pair<
		typename boost::range_const_iterator<ForwardRange>::type,
		typename boost::range_const_iterator<ForwardRange>::type
	>
		minmax_element(const ForwardRange & rng, Pred pred)
	{
		return varalgo::minmax_element(boost::begin(rng), boost::end(rng), pred);
	}

	template <class ForwardRange, class Pred>
	inline std::pair<
		typename boost::range_iterator<ForwardRange>::type,
		typename boost::range_iterator<ForwardRange>::type
	>
		minmax_element(ForwardRange & rng, Pred pred)
	{
		return varalgo::minmax_element(boost::begin(rng), boost::end(rng), pred);
	}
}}