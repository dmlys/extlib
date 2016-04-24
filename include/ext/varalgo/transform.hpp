#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/algorithm/transform.hpp> // for dual transform
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	/************************************************************************/
	/*              transform single                                        */
	/************************************************************************/
	template <class InputIterator, class OutputIterator>
	struct transform_visitor :
		boost::static_visitor<OutputIterator>
	{
		InputIterator first, last;
		OutputIterator dest;

		transform_visitor(InputIterator first, InputIterator last, OutputIterator dest)
			: first(first), last(last), dest(dest) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred) const
		{
			return std::transform(first, last, dest, pred);
		}
	};

	template <class InputIterator, class OutputIterator, class... VaraintTypes>
	inline OutputIterator
		transform(InputIterator first, InputIterator last, OutputIterator dest,
		          const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			transform_visitor<InputIterator, OutputIterator> {first, last, dest},
			pred);
	}
	
	template <class InputIterator, class OutputIterator, class Pred>
	inline OutputIterator
		transform(InputIterator first, InputIterator last, OutputIterator dest,
		          Pred pred)
	{
		return std::transform(first, last, dest, pred);
	}

	/// range overloads
	template <class SinglePassRange, class OutputIterator, class Pred>
	inline OutputIterator
		transform(const SinglePassRange & rng, OutputIterator dest, const Pred & pred)
	{
		return varalgo::transform(boost::begin(rng), boost::end(rng), dest, pred);
	}
}}