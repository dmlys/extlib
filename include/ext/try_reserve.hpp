#pragma once
#include <ext/type_traits.hpp>

namespace ext
{
	/// если категория итератора - std::random_access_iterator_tag
	/// вычисляет дистанцию между итераторами и вызывает reserve у container,
	/// иначе не делает ничего
	template <class Container, class Iterator>
	inline std::enable_if_t<ext::is_iterator_category_v<Iterator, std::random_access_iterator_tag>>
	try_reserve(Container & container, Iterator first, Iterator last)
	{
		container.reserve(last - first);
	}

	template <class Container, class Iterator>
	inline std::enable_if_t<not ext::is_iterator_category_v<Iterator, std::random_access_iterator_tag>>
	try_reserve(Container & container, Iterator first, Iterator last)
	{
	
	}

	template <class Iterator>
	inline auto try_distance(Iterator first, Iterator last) -> std::enable_if_t<ext::is_iterator_category_v<Iterator, std::random_access_iterator_tag>, decltype(last - first)>
	{
		return last - first;
	}

	template <class Iterator>
	inline auto try_distance(Iterator first, Iterator last) -> std::enable_if_t<not ext::is_iterator_category_v<Iterator, std::random_access_iterator_tag>, std::ptrdiff_t>
	{
		return 0;
	}
}
