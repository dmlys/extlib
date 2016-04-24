#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class ForwardIterator, class Type>
	struct replace_if_visitor :
		boost::static_visitor<void>
	{
		ForwardIterator first, last;
		const Type & new_val;

		replace_if_visitor(ForwardIterator first, ForwardIterator last, const Type & new_val)
			: first(first), last(last), new_val(new_val) {}

		template <class Pred>
		inline void operator()(Pred pred) const
		{
			std::replace_if(first, last, pred, new_val);
		}
	};

	template <class ForwardIterator, class Type, class... VaraintTypes>
	inline void
		replace_if(ForwardIterator first, ForwardIterator last,
		           const boost::variant<VaraintTypes...> & pred, const Type & new_val)
	{
		boost::apply_visitor(
			replace_if_visitor<ForwardIterator, Type> {first, last, new_val},
			pred);
	}
	
	template <class ForwardIterator, class Type, class Pred>
	inline void
		replace_if(ForwardIterator first, ForwardIterator last,
		           Pred pred, const Type & new_val)
	{
		std::replace_if(first, last, pred, new_val);
	}

	/// range overloads
	template <class ForwardRange, class Type, class Pred>
	inline const ForwardRange &
		replace_if(const ForwardRange & rng, const Pred & pred, const Type & new_val)
	{
		varalgo::replace_if(boost::begin(rng), boost::end(rng), pred, new_val);
		return rng;
	}

	template <class ForwardRange, class Type, class Pred>
	inline ForwardRange &
		replace_if(ForwardRange & rng, const Pred & pred, const Type & new_val)
	{
		varalgo::replace_if(boost::begin(rng), boost::end(rng), pred, new_val);
		return rng;
	}
}}