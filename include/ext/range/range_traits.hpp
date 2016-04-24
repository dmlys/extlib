#pragma once
#include <ext/type_traits.hpp>

#include <boost/predef.h>
#include <boost/utility/declval.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/iterator_range.hpp>

namespace ext
{
	/// функция обеспечивает обобщенный интерфейс data, как std::vector.data, std::array.data
	/// наподобие boost::range::begin
	/// здесь можно описывать метод data для контейнеров не имеющих его, но обладающих всем что бы его иметь
	template <class Range>
	inline auto data(Range & rng) -> decltype(rng.data())
	{
		return rng.data();
	}

	template <class Range>
	inline auto data(const Range & rng) -> decltype(rng.data())
	{
		return rng.data();
	}

	template <class Type>
	inline Type * data(Type * arr)
	{
		return arr;
	}

	template <class Type>
	inline Type * data(const boost::iterator_range<Type *> & rng)
	{
		return rng.begin();
	}

	template <class Type>
	inline Type * data(const std::pair<Type *, Type *> & rng)
	{
		return rng.first;
	}

#if BOOST_LIB_STD_DINKUMWARE
	// msvc не использует cow для строк, можно сделать const_cast
	template <class CharT, class traits, class allocator>
	inline CharT * data(std::basic_string<CharT, traits, allocator> & str)
	{
		return const_cast<CharT *>(str.data());
	}
#else
	template <class CharT, class traits, class allocator>
	inline CharT * data(std::basic_string<CharT, traits, allocator> & str)
	{
		return str.empty() ? nullptr : &str[0];
	}
#endif

	/// generic assign method, by default calls assign member function
	/// can be specialized/overloaded for containers/ranges which do not provide assign memeber method, like boost::iterator_range
	template <class Container, class Iterator>
	inline void assign(Container & rng, Iterator first, Iterator last)
	{
		rng.assign(first, last);
	}

	template <class RngIterator, class Iterator>
	inline void assign(boost::iterator_range<RngIterator> & rng, Iterator first, Iterator last)
	{
		rng = {first, last};
	}

	template <class RngIterator, class Iterator>
	inline void assign(boost::sub_range<RngIterator> & rng, Iterator first, Iterator last)
	{
		rng = {first, last};
	}

	template <class PairIterator, class Iterator>
	inline void assign(std::pair<PairIterator, PairIterator> & p, Iterator first, Iterator last)
	{
		p = {first, last};
	}

	/// generic append/range_push_back method, by default calls cont.insert(cont.end(), first, last)
	/// can be specialized/overloaded for containers/ranges which provides different method
	template <class Container, class Iterator>
	inline void append(Container & cont, Iterator first, Iterator last)
	{
		cont.insert(cont.end(), first, last);
	}

	template <class CharT, class traits, class allocator, class Iterator>
	inline void append(std::basic_string<CharT, traits, allocator> & str, Iterator first, Iterator last)
	{
		str.append(first, last);
	}

	/// generic clear method
	template <class Container>
	inline void clear(Container & cont)
	{
		cont.clear();
	}

	/************************************************************************/
	/*        range type traits                                             */
	/************************************************************************/
	namespace detail
	{
		template <class Type>
		struct has_resize_method_test
		{
			template<class C>
			static auto Test(int) -> decltype(boost::declval<C>().resize(0), detail::Yes());
			
			template<class C>
			static detail::No Test(...);

			typedef typename std::decay<Type>::type DecayedType;
			static const bool value = (sizeof(Test<DecayedType>(0)) == sizeof(detail::Yes));
		};

		template <typename Type>
		struct is_contiguous_container_test
		{
			template <class C>
			static auto Test(int) -> decltype(ext::data(boost::declval<C>()), detail::Yes());
			
			template <class C>
			static detail::No Test(...);

			typedef typename std::decay<Type>::type DecayedType;
			static const bool value = (sizeof(Test<DecayedType>(0)) == sizeof(detail::Yes));
		};

		/// скорее всего теста boost::has_range_iterator<Type>::type должно быть достаточно
		template <typename Type, class = typename boost::has_range_iterator<Type>::type>
		struct is_begin_expression_valid_test
		{
			static const bool value = false;
		};

		template <typename Type>
		struct is_begin_expression_valid_test<Type, boost::mpl::true_>
		{
			template <class C, class = decltype(boost::begin(boost::declval<C>()))>
			static detail::Yes Test(int);

			template <class C>
			static detail::No Test(...);

			typedef typename std::remove_cv<typename std::remove_reference<Type>::type>::type DecayedType;
			static const bool value = sizeof(Test<DecayedType>(0)) == sizeof(detail::Yes);
		};

		/// скорее всего теста boost::has_range_iterator<Type>::type должно быть достаточно
		template <typename Type, class = typename boost::has_range_iterator<Type>::type>
		struct is_end_expression_valid_test
		{
			static const bool value = false;
		};

		template <typename Type>
		struct is_end_expression_valid_test<Type, boost::mpl::true_>
		{
			template <class C, class = decltype(boost::end(boost::declval<C>()))>
			static detail::Yes Test(int);

			template <class C>
			static detail::No Test(...);

			typedef typename std::remove_cv<typename std::remove_reference<Type>::type>::type DecayedType;
			static const bool value = sizeof(Test<DecayedType>(0)) == sizeof(detail::Yes);
		};


		template <class Type, typename Range, class = typename boost::has_range_iterator<Range>::type>
		struct is_range_of_test
		{
			static const bool value = false;
		};

		template <class Type, typename Range>
		struct is_range_of_test<Type, Range, boost::mpl::true_>
		{
			static const bool value = std::is_same<Type, typename boost::range_value<Range>::type>::value;
		};
	}

	/// checks is there is resize method on type Type
	template <typename Type>
	struct has_resize_method :
		std::integral_constant<bool,
			detail::has_resize_method_test<Type>::value
		>
	{};

	/// determines if container is contiguous
	/// if expression: ext::data(declval<Type>()) is valid - it is contiguous container
	/// examples of such containers is: vector, string, array. But not std::deque
	template <typename Type>
	struct is_contiguous_container :
		std::integral_constant<bool,
			detail::is_contiguous_container_test<Type>::value
		>
	{};

	template <typename Type>
	struct is_begin_expression_valid :
		std::integral_constant<bool,
			detail::is_begin_expression_valid_test<Type>::value
		>
	{};

	template <typename Type>
	struct is_end_expression_valid :
		std::integral_constant<bool,
			detail::is_end_expression_valid_test<Type>::value
		>
	{};

	template <typename Type>
	struct is_range :
		std::integral_constant<bool,
			is_begin_expression_valid<Type>::value &&
			is_end_expression_valid<Type>::value
		>
	{};

	template <typename Range, typename Type>
	struct is_range_of :
		std::integral_constant<bool,
			is_range<Range>::value &&
			detail::is_range_of_test<Type, Range>::value
		>
	{};
}