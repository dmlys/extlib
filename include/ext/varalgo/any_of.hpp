#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator>
	struct any_of_visitor :
		boost::static_visitor<bool>
	{
		InputIterator first, last;

		any_of_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::any_of(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline bool
		any_of(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			any_of_visitor<InputIterator> {first, last},
			pred);
	}

	template <class InputIterator, class Pred>
	inline bool
		any_of(InputIterator first, InputIterator last, Pred pred)
	{
		return std::any_of(first, last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class Pred>
	inline bool
		any_of(const SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::any_of(boost::begin(rng), boost::end(rng), pred);
	}
}}