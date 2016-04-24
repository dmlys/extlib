#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator, class OutputIterator, class Type>
	struct replace_copy_if_visitor :
		boost::static_visitor<OutputIterator>
	{
		InputIterator first, last;
		OutputIterator dest;
		const Type & new_val;

		replace_copy_if_visitor(InputIterator first, InputIterator last, OutputIterator dest, const Type & new_val)
			: first(first), last(last), dest(dest), new_val(new_val) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred) const
		{
			return std::replace_copy_if(first, last, dest, pred, new_val);
		}
	};

	template <class InputIterator, class OutputIterator, class Type, class... VaraintTypes>
	inline OutputIterator
		replace_copy_if(InputIterator first, InputIterator last, OutputIterator dest,
		                const boost::variant<VaraintTypes...> & pred, const Type & new_val)
	{
		return boost::apply_visitor(
			replace_copy_if_visitor<InputIterator, OutputIterator, Type> {first, last, dest, new_val},
			pred);
	}
	
	template <class InputIterator, class OutputIterator, class Type, class Pred>
	inline OutputIterator
		replace_copy_if(InputIterator first, InputIterator last, OutputIterator dest,
		                Pred pred, const Type & new_val)
	{
		return std::replace_copy_if(first, last, dest, pred, new_val);
	}

	/// range overloads
	template <class SinglePassRange, class OutputIterator, class Type, class Pred>
	inline OutputIterator
		replace_copy_if(const SinglePassRange & rng, OutputIterator dest, const Pred & pred, const Type & new_val)
	{
		return varalgo::replace_copy_if(boost::begin(rng), boost::end(rng), dest, pred, new_val);
	}
}}