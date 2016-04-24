#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator>
	struct none_of_visitor :
		boost::static_visitor<bool>
	{
		InputIterator first, last;

		none_of_visitor(InputIterator first, InputIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::none_of(first, last, pred);
		}
	};

	template <class InputIterator, class... VaraintTypes>
	inline bool
		none_of(InputIterator first, InputIterator last, const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			none_of_visitor<InputIterator> {first, last},
			pred);
	}

	template <class InputIterator, class Pred>
	inline bool
		none_of(InputIterator first, InputIterator last, Pred pred)
	{
		return std::none_of(first, last, pred);
	}

	/// range overloads
	template <class SinglePassRange, class Pred>
	inline bool
		none_of(const SinglePassRange & rng, const Pred & pred)
	{
		return varalgo::none_of(boost::begin(rng), boost::end(rng), pred);
	}
}}