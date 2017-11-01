#pragma once
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


	namespace detail
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
	struct is_iterator : detail::is_iterator<Type> {};

	template <class Type>
	constexpr bool is_iterator_v = is_iterator<Type>::value;



	template <class Target, class Type, class = void>
	struct static_castable : std::false_type {};

	template <class Target, class Type>
	struct static_castable<Target, Type, ext::void_t<decltype(static_cast<Target>(std::declval<Type>()))>>
		: std::true_type {};

	template <class Target, class Type, class = void>
	struct dynamic_castable : std::false_type {};

	template <class Target, class Type>
	struct dynamic_castable<Target, Type, ext::void_t<decltype(dynamic_cast<Target>(std::declval<Type>()))>>
		: std::true_type {};

	template <class Target, class Type, class = void>
	struct reinterpret_castable : std::false_type {};

	template <class Target, class Type>
	struct reinterpret_castable<Target, Type, ext::void_t<decltype(reinterpret_cast<Target>(std::declval<Type>()))>>
		: std::true_type {};


	template <class Target, class Type>
	constexpr bool static_castable_v = static_castable<Target, Type>::value;

	template <class Target, class Type>
	constexpr bool dynamic_castable_v = dynamic_castable<Target, Type>::value;

	template <class Target, class Type>
	constexpr bool reinterpret_castable_v = reinterpret_castable<Target, Type>::value;
}
