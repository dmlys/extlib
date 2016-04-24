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
	struct for_each_visitor :
		boost::static_visitor<void>
	{
		InputIterator first, last;

		for_each_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::for_each(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline void
		for_each(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		boost::apply_visitor(
			for_each_visitor<InputIterator> {first, last},
			pred);
	}
	
	template <class InputIterator, class Pred>
	inline Pred
		for_each(InputIterator first, InputIterator last, Pred pred)
	{
		return std::for_each(first, last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class Pred>
	inline void
		for_each(const SinglePassRange & rng, const Pred & pred)
	{
		varalgo::for_each(boost::begin(rng), boost::end(rng), pred);
	}

	template <class SinglePassRange, class Pred>
	inline void
		for_each(SinglePassRange & rng, const Pred & pred)
	{
		varalgo::for_each(boost::begin(rng), boost::end(rng), pred);
	}
}}