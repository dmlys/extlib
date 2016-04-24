#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/algorithm/cxx14/mismatch.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo{

	template <class InputIterator, class OutputIterator1, class OutputIterator2>
	struct partition_copy_visitor :
		boost::static_visitor<std::pair<OutputIterator1, OutputIterator2>>
	{
		InputIterator first, last;
		OutputIterator1 dest_true;
		OutputIterator2 dest_false;

		partition_copy_visitor(InputIterator first, InputIterator last,
		                       OutputIterator1 dest_true, OutputIterator2 dest_false)
			: first(first), last(last), dest_true(dest_true), dest_false(dest_false) {}

		template <class Pred>
		inline std::pair<OutputIterator1, OutputIterator2> operator()(Pred pred) const
		{
			return std::partition_copy(first, last, dest_true, dest_false, pred);
		}
	};


	template <class InputIterator, class OutputIterator1, class OutputIterator2, class... VaraintTypes>
	inline std::pair<OutputIterator1, OutputIterator2>
		partition_copy(InputIterator first, InputIterator last,
		               OutputIterator1 dest_true, OutputIterator2 dest_false,
		               const boost::variant<VaraintTypes...> & pred)
	{
		return boost::apply_visitor(
			partition_copy_visitor < InputIterator, OutputIterator1, OutputIterator2> {first, last, dest_true, dest_false},
			pred);
	}

	template <class InputIterator, class OutputIterator1, class OutputIterator2, class Pred>
	inline std::pair<OutputIterator1, OutputIterator2>
		partition_copy(InputIterator first, InputIterator last,
		               OutputIterator1 dest_true, OutputIterator2 dest_false,
		               Pred pred)
	{
		return std::partition_copy(first, last, dest_true, dest_false, pred);
	}

	/// range overloads
	template <class SinglePassRange, class OutputIterator1, class OutputIterator2, class Pred>
	inline std::pair<OutputIterator1, OutputIterator2>
		partition_copy(const SinglePassRange & rng,
		               OutputIterator1 dest_true, OutputIterator2 dest_false,
		               const Pred & pred)
	{
		return varalgo::partition_copy(boost::begin(rng), boost::end(rng), dest_true, dest_false, pred);
	}
}}