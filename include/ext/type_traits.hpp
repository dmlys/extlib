#pragma once
#include <iterator>
#include <type_traits>

namespace ext
{
	/// some useful stuff for any type_traits

	/// Walter E. Brown trick
	template <class ...>
	struct voider
	{
		typedef void type;
	};

	template <class... Pack>
	using void_t = typename voider<Pack...>::type;

	namespace detail
	{
		struct Yes { char unused[1]; };
		struct No { char unused[2]; };
		static_assert(sizeof(Yes) != sizeof(No), "");
	}

	template <class Type>
	struct remove_cvref
	{
		using type = std::remove_cv_t<std::remove_reference_t<Type>>;
	};

	template <class Type>
	using remove_cvref_t = typename remove_cvref<Type>::type;


	namespace iterator_detail
	{
		template <class Type, class = void>
		struct is_iterator : std::false_type {};

		template <class Type>
		struct is_iterator<Type, void_t<typename Type::iterator_category>> : std::true_type {};

		template <class Type>
		struct is_iterator<Type *> : std::true_type {};

		template <class Type>
		struct is_iterator<const Type *> : std::true_type {};

		template <class Type>
		struct is_iterator<Type[]> : std::true_type {};

		template <class Type, std::size_t N>
		struct is_iterator<Type[N]> : std::true_type {};
	}

	template <class Type>
	struct is_iterator : ext::iterator_detail::is_iterator<Type> {};

	template <class Type>
	constexpr bool is_iterator_v = is_iterator<Type>::value;



	template <class Iterator, class Default = void, bool = ext::is_iterator<Iterator>::value>
	struct iterator_value
	{
		using type = Default;
	};

	template <class Iterator, class Default>
	struct iterator_value<Iterator, Default, true>
	{
		using type = typename std::iterator_traits<Iterator>::value_type;
	};

	template <class Iterator, class Default = void>
	using iterator_value_t = typename iterator_value<Iterator, Default>::type;

	template <class Iterator, class Type>
	struct is_iterator_of :
		std::is_same<iterator_value_t<Iterator>, Type> {};

	template <class Iterator, class Type>
	constexpr auto is_iterator_of_v = is_iterator_of<Iterator, Type>::value;


	template <class Iterator, class Default = void, bool = ext::is_iterator<Iterator>::value>
	struct iterator_category
	{
		using type = Default;
	};

	template <class Iterator, class Default>
	struct iterator_category<Iterator, Default, true>
	{
		using type = typename std::iterator_traits<Iterator>::iterator_category;
	};

	template <class Iterator, class Default = void>
	using iterator_category_t = typename iterator_category<Iterator, Default>::type;

	template <class Iterator, class Category>
	struct is_iterator_category :
		std::is_convertible<ext::iterator_category_t<Iterator>, Category> {};

	template <class Iterator, class Category>
	constexpr auto is_iterator_category_v = is_iterator_category<Iterator, Category>::value;


	template <class From, class To, class = void>
	struct static_castable : std::false_type {};

	template <class From, class To>
	struct static_castable<From, To, ext::void_t<decltype(static_cast<To>(std::declval<From>()))>>
		: std::true_type {};

	template <class From, class To, class = void>
	struct dynamic_castable : std::false_type {};

	template <class From, class To>
	struct dynamic_castable<From, To, ext::void_t<decltype(dynamic_cast<To>(std::declval<From>()))>>
		: std::true_type {};

	template <class From, class To, class = void>
	struct reinterpret_castable : std::false_type {};

	template <class From, class To>
	struct reinterpret_castable<From, To, ext::void_t<decltype(reinterpret_cast<To>(std::declval<From>()))>>
		: std::true_type {};


	template <class From, class To>
	constexpr bool static_castable_v = static_castable<From, To>::value;

	template <class From, class To>
	constexpr bool dynamic_castable_v = dynamic_castable<From, To>::value;

	template <class From, class To>
	constexpr bool reinterpret_castable_v = reinterpret_castable<From, To>::value;
}
