#pragma once
#include <type_traits>
#include <tuple>

#include <boost/range.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <ext/utility.hpp>

namespace ext
{
	namespace detail
	{
		template <std::size_t Idx, class ForwardRange>
		struct get_func_iterator_types
		{
			// The main problem here is that: get_func<Idx> behaves the same as get function call:
			// it always returns reference to a part of tuple-like object, which is ok.
			//
			// But in our case: if underlying iterator returns object by value -
			// we must return part of that tuple by value to, or we'll get dangling reference - nothing good

			using range_iterator = typename boost::range_iterator<ForwardRange>::type;
			using range_iterator_reference = typename boost::iterator_reference<range_iterator>::type;

			using result_type = std::result_of_t<get_func<Idx>(range_iterator_reference)>;
			using element_type = std::tuple_element_t<Idx, std::decay_t<range_iterator_reference>>;
			
			// if tuple we have - is a tuple lreference or element is a lreference - we can pass as is,
			// three is no dangling reference, otherwise - we should pass by value.
			using reference = std::conditional_t<
				std::is_lvalue_reference_v<range_iterator_reference> or std::is_lvalue_reference_v<element_type>,
				result_type,
				std::remove_reference_t<result_type>
			>;

			using value_type = std::remove_reference_t<reference>;

			using iterator = boost::transform_iterator<
				get_func<Idx>, range_iterator, reference, value_type
			>;
		};


		template <class ForwardRange, std::size_t Idx>
		struct getted_range : boost::iterator_range<typename get_func_iterator_types<Idx, ForwardRange>::iterator>
		{
		private:
			using base_type = boost::iterator_range<typename get_func_iterator_types<Idx, ForwardRange>::iterator>;

		public:
			typedef typename base_type::iterator iterator;

			getted_range(ForwardRange & rng)
				: base_type(iterator(boost::begin(rng)), iterator(iterator(boost::end(rng)))) {}
		};

		
		template <std::size_t Idx>
		struct getted_type_tag
		{
			static constexpr std::size_t idx = Idx;
		};


		template <class ForwardRange, std::size_t Idx>
		getted_range<ForwardRange, Idx> operator | (ForwardRange & rng, getted_type_tag<Idx>)
		{
			return getted_range<ForwardRange, Idx>(rng);
		}

		//template <class ForwardRange, std::size_t Idx>
		//getted_range<const ForwardRange, Idx> operator | (const ForwardRange & rng, getted_type_tag<Idx>)
		//{
		//	return getted_range<ForwardRange, Idx>(rng);
		//}
	}

	template <std::size_t Idx>
	constexpr detail::getted_type_tag<Idx> getted {};

	constexpr detail::getted_type_tag<0> firsts;
	constexpr detail::getted_type_tag<1> seconds;
}
