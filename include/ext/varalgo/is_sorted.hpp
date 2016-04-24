#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class ForwardIterator>
	struct is_sorted_visitor : boost::static_visitor<bool>
	{
		ForwardIterator first, last;
		is_sorted_visitor(ForwardIterator first, ForwardIterator last)
			: first(first), last(last) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::is_sorted(first, last, pred);
		}
	};

	template <class ForwardIterator, class... VariantTypes>
	inline bool is_sorted(ForwardIterator first, ForwardIterator last, const
	                      boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(is_sorted_visitor<ForwardIterator> {first, last}, pred);
	}

	template <class ForwardIterator, class Pred>
	inline bool is_sorted(ForwardIterator first, ForwardIterator last, const
	                      Pred pred)
	{
		return std::is_sorted(first, last, pred);
	}

	template <class ForwardRange, class Pred>
	inline bool is_sorted(const ForwardRange & rng, const Pred & pred)
	{
		return varalgo::is_sorted(boost::begin(rng), boost::end(rng), pred);
	}
}}

