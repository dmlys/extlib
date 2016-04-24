#pragma once
#include <algorithm>
#include <ext/range/range_traits.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/range/functions.hpp>

namespace ext
{
	///аналогичен std::binary_search, но в отличии от возвращают итератор(если не найден - end)
	template <class FwdIterator, class Type>
	FwdIterator binary_find(FwdIterator beg, FwdIterator end, const Type & val)
	{
		beg = std::lower_bound(beg, end, val);          // !(el < val)
		return beg != end && !(val < *beg) ? beg : end; // !(val < el) => el === val
	}

	///аналогичен std::binary_search, но в отличии от возвращают итератор(если не найден - end)
	template <class FwdIterator, class Type, class Pred>
	FwdIterator binary_find(FwdIterator beg, FwdIterator end, const Type & val, Pred pred)
	{
		beg = std::lower_bound(beg, end, val, pred);         // !pred(el, val)
		return beg != end && !(pred(val, *beg)) ? beg : end; // !pred(val, el) => el === val
	}

	/// \overload
	template <class ForwardRange, class Type>
	typename boost::range_iterator<ForwardRange>::type
	binary_find(ForwardRange & rng, const Type & val)
	{
		return binary_find(boost::begin(rng), boost::end(rng), val);
	}

	/// \overload
	template <class ForwardRange, class Type>
	typename boost::range_iterator<const ForwardRange>::type
	binary_find(const ForwardRange & rng, const Type & val)
	{
		return binary_find(boost::begin(rng), boost::end(rng), val);
	}

	/// \overload
	// SFINAE to resolve conflict with overload(first, last, val)
	// it least on msvc 2013 it conflicts
	template <
		class ForwardRange, class Type, class Pred,
		typename std::enable_if<ext::is_range<ForwardRange>::value, int>::type = 0
	>
	typename boost::range_iterator<ForwardRange>::type
	binary_find(ForwardRange & rng, const Type & val, Pred pred)
	{
		return binary_find(boost::begin(rng), boost::end(rng), val, pred);
	}

	/// \overload
	// SFINAE to resolve conflict with overload(first, last, val)
	// it least on msvc 2013 it conflicts
	template <
		class ForwardRange, class Type, class Pred,
		typename std::enable_if<ext::is_range<ForwardRange>::value, int>::type = 0
	>
	typename boost::range_iterator<ForwardRange>::type
	binary_find(const ForwardRange & rng, const Type & val, Pred pred)
	{
		return binary_find(boost::begin(rng), boost::end(rng), val, pred);
	}
}