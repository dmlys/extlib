#pragma once
#include <cstddef>
#include <utility>
#include <tuple>

namespace ext::container
{
	template <class Type, class Allocator>
	class simple_vector_storage : Allocator
	{
	public:
		using value_type = Type;
		using size_type = std::size_t;
		using pointers_tuple = std::tuple<value_type *, value_type *, value_type *>;
		using allocator_type = Allocator;

	public:
		value_type * m_first = nullptr;
		value_type * m_last = nullptr;
		value_type * m_stotage_end = nullptr;

	public:
		value_type * get_first()    const noexcept  { return m_first; }
		value_type * get_last()     const noexcept  { return m_last; }
		size_type    get_capacity() const noexcept  { return m_stotage_end; }
		size_type    get_size()     const noexcept  { return m_last - m_first; }

		pointers_tuple get_storage_pointers() const noexcept { return {m_first, m_last, m_stotage_end}; }
		void set_storage_pointers(pointers_tuple pointers) noexcept { std::tie(m_first, m_last, m_stotage_end) = pointers; }

	public:
		void set_allocator(const allocator_type & alloc) { static_cast<allocator_type &>(*this) = alloc; };
		const allocator_type & get_allocator() const noexcept { return static_cast<const allocator_type &>(*this); }
		      allocator_type & get_allocator()       noexcept { return static_cast<      allocator_type &>(*this); }

	public:
		simple_vector_storage() noexcept = default;
		simple_vector_storage(const allocator_type & alloc) noexcept : allocator_type(alloc) {};

		simple_vector_storage(simple_vector_storage &&) noexcept;
		simple_vector_storage & operator =(simple_vector_storage &&) noexcept;
	};

	template <class Type, class Allocator>
	inline simple_vector_storage<Type, Allocator>::simple_vector_storage(simple_vector_storage && other) noexcept
	    : m_first(std::exchange(other.m_first, nullptr)),
	      m_last(std::exchange(other.m_last, nullptr)),
	      m_stotage_end(std::exchange(other.m_stotage_end, nullptr))
	{

	}

	template <class Type, class Allocator>
	inline simple_vector_storage<Type, Allocator> & simple_vector_storage<Type, Allocator>::operator=(simple_vector_storage && other) noexcept
	{
		this->~simple_vector_storage();
		new(this) simple_vector_storage(std::move(other));

		return *this;
	}
}
