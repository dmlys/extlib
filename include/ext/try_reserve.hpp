#pragma once
#include <type_traits>
#include <iterator>

namespace ext
{
	/// если категория итератора - std::random_access_iterator_tag
	/// вычисляет дистанцию между итераторами и вызывает reserve у container,
	/// иначе не делает ничего
	template <class Container, class Iterator>
	inline
	typename std::enable_if<
		std::is_convertible<
			std::random_access_iterator_tag,
			typename std::iterator_traits<Iterator>::iterator_category
		>::value
	>::type
	try_reserve(Container & container, Iterator first, Iterator last)
	{
		container.reserve(last - first);
	}

	template <class Container, class Iterator>
	inline
	typename std::enable_if<
		! std::is_convertible<
			std::random_access_iterator_tag,
			typename std::iterator_traits<Iterator>::iterator_category
		>::value
	>::type
	try_reserve(Container & container, Iterator first, Iterator last)
	{
	
	}
}