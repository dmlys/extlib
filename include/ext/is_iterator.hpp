#pragma once
#include <ext/type_traits.hpp>

namespace ext
{
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
}
