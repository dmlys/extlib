#pragma once

#include <algorithm>

#include <boost/range.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace ext {
namespace varalgo {

	/************************************************************************/
	/*                     includes                                         */
	/************************************************************************/
	template <class InputIterator1, class InputIterator2>
	struct includes_visitor : boost::static_visitor<bool>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;

		includes_visitor(InputIterator1 first1, InputIterator1 last1,
		                 InputIterator2 first2, InputIterator2 last2)
			: first1(first1), last1(last1), first2(first2), last2(last2) {}

		template <class Pred>
		inline bool operator()(Pred pred) const
		{
			return std::includes(first1, last1, first2, last2, pred);
		}
	};

	template <class InputIterator1, class InputIterator2, class... VariantTypes>
	inline bool includes(InputIterator1 first1, InputIterator1 last1,
	                     InputIterator2 first2, InputIterator2 last2,
	                     const boost::variant<VariantTypes...> & pred)
	{
		typedef includes_visitor<InputIterator1, InputIterator2> visitor;
		return boost::apply_visitor(
			visitor {first1, last1, first2, last2},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class Pred>
	inline bool includes(InputIterator1 first1, InputIterator1 last1,
	                     InputIterator2 first2, InputIterator2 last2,
	                     Pred pred)
	{
		return std::includes(first1, last1, first2, last2, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class Pred>
	inline bool includes(const SinglePassRange1 & rng1, const SinglePassRange2 & rng2, const Pred & pred)
	{
		return varalgo::
			includes(boost::begin(rng1), boost::end(rng1),
			         boost::begin(rng2), boost::end(rng2),
			         pred);
	}

	/************************************************************************/
	/*                       set_difference                                 */
	/************************************************************************/
	template <class InputIterator1, class InputIterator2, class OutputIterator>
	struct set_difference_visitor : boost::static_visitor<OutputIterator>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;
		OutputIterator out;

		set_difference_visitor(InputIterator1 first1, InputIterator1 last1,
		                       InputIterator2 first2, InputIterator2 last2,
		                       OutputIterator out)
			: first1(first1), last1(last1), first2(first2), last2(last2), out(out) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred)
		{
			return std::set_difference(first1, last1, first2, last2, out, pred);
		}
	};
	
	template <class InputIterator1, class InputIterator2, class OutputIterator, class... VariantTypes>
	inline OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
	                                     InputIterator2 first2, InputIterator2 last2,
	                                     OutputIterator out,
	                                     const boost::variant<VariantTypes...> & pred)
	{
		typedef set_difference_visitor<InputIterator1, InputIterator2, OutputIterator> visitor;
		return boost::apply_visitor(
			visitor {first1, last1, first2, last2, out},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class OutputIterator, class Pred>
	inline OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
	                                     InputIterator2 first2, InputIterator2 last2,
	                                     OutputIterator out,
	                                     Pred pred)
	{
		return std::set_difference(first1, last1, first2, last2, out, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class OutputIterator, class Pred>
	inline bool set_difference(const SinglePassRange1 & rng1,
	                           const SinglePassRange2 & rng2,
	                           OutputIterator out,
	                           const Pred & pred)
	{
		return varalgo::
			set_difference(boost::begin(rng1), boost::end(rng1),
			               boost::begin(rng2), boost::end(rng2),
			               out, pred);
	}

	/************************************************************************/
	/*                       set_intersection                               */
	/************************************************************************/
	template <class InputIterator1, class InputIterator2, class OutputIterator>
	struct set_intersection_visitor : boost::static_visitor<OutputIterator>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;
		OutputIterator out;

		set_intersection_visitor(InputIterator1 first1, InputIterator1 last1,
		                         InputIterator2 first2, InputIterator2 last2,
		                         OutputIterator out)
			: first1(first1), last1(last1), first2(first2), last2(last2), out(out) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred)
		{
			return std::set_intersection(first1, last1, first2, last2, out, pred);
		}
	};
	
	template <class InputIterator1, class InputIterator2, class OutputIterator, class... VariantTypes>
	inline OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
	                                       InputIterator2 first2, InputIterator2 last2,
	                                       OutputIterator out,
	                                       const boost::variant<VariantTypes...> & pred)
	{
		typedef set_intersection_visitor<InputIterator1, InputIterator2, OutputIterator> visitor;
		return boost::apply_visitor(
			visitor {first1, last1, first2, last2, out},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class OutputIterator, class Pred>
	inline OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
	                                       InputIterator2 first2, InputIterator2 last2,
	                                       OutputIterator out,
	                                       Pred pred)
	{
		return std::set_intersection(first1, last1, first2, last2, out, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class OutputIterator, class Pred>
	inline bool set_intersection(const SinglePassRange1 & rng1,
	                             const SinglePassRange2 & rng2,
	                             OutputIterator out,
	                             const Pred & pred)
	{
		return varalgo::
			set_intersection(boost::begin(rng1), boost::end(rng1),
			                 boost::begin(rng2), boost::end(rng2),
			                 out, pred);
	}

	/************************************************************************/
	/*                       set_symmetric_difference                       */
	/************************************************************************/
	template <class InputIterator1, class InputIterator2, class OutputIterator>
	struct set_symmetric_difference_visitor : boost::static_visitor<OutputIterator>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;
		OutputIterator out;

		set_symmetric_difference_visitor(InputIterator1 first1, InputIterator1 last1,
		                                 InputIterator2 first2, InputIterator2 last2,
		                                 OutputIterator out)
			: first1(first1), last1(last1), first2(first2), last2(last2), out(out) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred)
		{
			return std::set_symmetric_difference(first1, last1, first2, last2, out, pred);
		}
	};
	
	template <class InputIterator1, class InputIterator2, class OutputIterator, class... VariantTypes>
	inline OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
	                                               InputIterator2 first2, InputIterator2 last2,
	                                               OutputIterator out,
	                                               const boost::variant<VariantTypes...> & pred)
	{
		typedef set_symmetric_difference_visitor<InputIterator1, InputIterator2, OutputIterator> visitor;
		return boost::apply_visitor(
			visitor {first1, last1, first2, last2, out},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class OutputIterator, class Pred>
	inline OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
	                                               InputIterator2 first2, InputIterator2 last2,
	                                               OutputIterator out,
	                                               Pred pred)
	{
		return std::set_symmetric_difference(first1, last1, first2, last2, out, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class OutputIterator, class Pred>
	inline bool set_symmetric_difference(const SinglePassRange1 & rng1,
	                                     const SinglePassRange2 & rng2,
	                                     OutputIterator out,
	                                     const Pred & pred)
	{
		return varalgo::
			set_symmetric_difference(boost::begin(rng1), boost::end(rng1),
			                         boost::begin(rng2), boost::end(rng2),
			                         out, pred);
	}

	/************************************************************************/
	/*                       set_union                                      */
	/************************************************************************/
	template <class InputIterator1, class InputIterator2, class OutputIterator>
	struct set_union_visitor : boost::static_visitor<OutputIterator>
	{
		InputIterator1 first1, last1;
		InputIterator2 first2, last2;
		OutputIterator out;

		set_union_visitor(InputIterator1 first1, InputIterator1 last1,
		                  InputIterator2 first2, InputIterator2 last2,
		                  OutputIterator out)
			: first1(first1), last1(last1), first2(first2), last2(last2), out(out) {}

		template <class Pred>
		inline OutputIterator operator()(Pred pred)
		{
			return std::set_union(first1, last1, first2, last2, out, pred);
		}
	};
	
	template <class InputIterator1, class InputIterator2, class OutputIterator, class... VariantTypes>
	inline OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
	                                InputIterator2 first2, InputIterator2 last2,
	                                OutputIterator out,
	                                const boost::variant<VariantTypes...> & pred)
	{
		typedef set_union_visitor<InputIterator1, InputIterator2, OutputIterator> visitor;
		return boost::apply_visitor(
			visitor {first1, last1, first2, last2, out},
			pred);
	}

	template <class InputIterator1, class InputIterator2, class OutputIterator, class Pred>
	inline OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
	                                InputIterator2 first2, InputIterator2 last2,
	                                OutputIterator out,
	                                Pred pred)
	{
		return std::set_union(first1, last1, first2, last2, out, pred);
	}

	/// range overloads
	template <class SinglePassRange1, class SinglePassRange2, class OutputIterator, class Pred>
	inline bool set_union(const SinglePassRange1 & rng1,
	                      const SinglePassRange2 & rng2,
	                      OutputIterator out,
	                      const Pred & pred)
	{
		return varalgo::
			set_union(boost::begin(rng1), boost::end(rng1),
			          boost::begin(rng2), boost::end(rng2),
			          out, pred);
	}

}}
