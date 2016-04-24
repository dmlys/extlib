#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator, class OutputIterator>
	struct copy_if_visitor :
		boost::static_visitor<OutputIterator>
	{
		InputIterator first, last;
		OutputIterator dest;

		copy_if_visitor(InputIterator first, InputIterator last, OutputIterator dest)
			: first(first), last(last), dest(dest) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred) const
		{
			return std::copy_if(first, last, dest, pred);
		}
	};

	template <class InputIterator, class OutputIterator, class... VaraintTypes>
	inline OutputIterator
		copy_if(InputIterator first, InputIterator last, OutputIterator dest,
		        const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			copy_if_visitor<InputIterator, OutputIterator> {first, last, dest},
			pred);
	}
	
	template <class InputIterator, class OutputIterator, class Pred>
	inline OutputIterator
		copy_if(InputIterator first, InputIterator last, OutputIterator dest,
		        Pred pred)
	{
		return std::copy_if(first, last, dest, pred);
	}

	/// range overloads
	template <class SinglePassRange, class OutputIterator, class Pred>
	inline OutputIterator
		copy_if(const SinglePassRange & rng, OutputIterator dest, const Pred & pred)
	{
		return varalgo::copy_if(boost::begin(rng), boost::end(rng), dest, pred);
	}
}}