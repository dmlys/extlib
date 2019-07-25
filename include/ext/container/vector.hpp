#pragma once
#include <ext/container/vector_facade.hpp>
#include <ext/container/simple_vector_storage.hpp>
#include <ext/container/cow_vector_storage.hpp>

namespace ext::container
{
	template <class Type, class Allocator = std::allocator<Type>>
	using vector = vector_facade<simple_vector_storage<Type, Allocator>>;

	template <class Type, class Allocator = std::allocator<Type>>
	using cow_vector = vector_facade<cow_vector_storage<Type, Allocator>>;
}
