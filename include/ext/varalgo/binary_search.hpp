#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo
{
	template <class ForwardIterator, class T>
	struct binary_search_visitor : boost::static_visitor<bool>
	{
		ForwardIterator first, last;
		const T & val;

		binary_search_visitor(ForwardIterator first, ForwardIterator last, const T & val)
			: first(first), last(last), val(val) {}
		
		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::binary_search(first, last, val, pred);
		}
	};

	template <class ForwardIterator, class Type, class... VariantTypes>
	inline bool binary_search(ForwardIterator first, ForwardIterator last,
	                          const Type & val, const boost::variant<VariantTypes...> & pred)
	{
		return boost::apply_visitor(
			binary_search_visitor<ForwardIterator, Type> {first, last, val},
			pred);
	}

	template <class ForwardIterator, class Type, class Pred>
	inline bool binary_search(ForwardIterator first, ForwardIterator last,
	                          const Type & val, Pred pred)
	{
		return std::binary_search(first, last, val, pred);
	}

	/// range overloads
	template <class ForwardRange, class Type, class Pred>
	inline bool binary_search(const ForwardRange & rng, const Type & val, const Pred & pred)
	{
		return varalgo::binary_search(boost::begin(rng), boost::end(rng), val, pred);
	}

	template <class ForwardRange, class Type, class Pred>
	inline bool binary_search(ForwardRange & rng, const Type & val, Pred & pred)
	{
		return varalgo::binary_search(boost::begin(rng), boost::end(rng), val, pred);
	}
}}
