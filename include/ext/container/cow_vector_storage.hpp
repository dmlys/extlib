#pragma once
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <tuple>
#include <type_traits>

#include <ext/intrusive_ptr.hpp>
#include <ext/container/uninitialized_algo.hpp>

namespace ext::container
{
	template <class storage>
	class vector_storage_traits;

	template <class storage>
	class vector_facade;

	template <class Type>
	struct cow_heap_body
	{
		unsigned refs = 1;
		unsigned size, capacity;
		Type elements[];


		static void destruct(cow_heap_body * ptr);
		static auto init_shared_null() -> ext::intrusive_cow_ptr<cow_heap_body>;

		static std::aligned_storage_t<sizeof(Type), alignof(Type)> ms_storage;
		static ext::intrusive_cow_ptr<cow_heap_body<Type>> ms_shared_null;
	};

	template <class Type>
	std::aligned_storage_t<sizeof(Type), alignof(Type)> cow_heap_body<Type>::ms_storage;

	template <class Type>
	ext::intrusive_cow_ptr<cow_heap_body<Type>> cow_heap_body<Type>::ms_shared_null = init_shared_null();




	template <class Type, class Allocator>
	class cow_vector_storage
	{
		using self_type = cow_vector_storage;
		friend vector_storage_traits<self_type>;

	public:
		using allocator_type = Allocator;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using value_type = Type;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type *;
		using const_pointer = const value_type *;

		using pointers_tuple = std::tuple<pointer, pointer, pointer>;
		using const_pointers_tuple = std::tuple<const_pointer, const_pointer, const_pointer>;

		//using size_type = typename std::allocator_traits<Allocator>::size_type;
		//using difference_type = typename std::allocator_traits<Allocator>::difference_type;

	private:
		using heap_body = cow_heap_body<value_type>;
		ext::intrusive_cow_ptr<heap_body> m_body;

	private:
		static heap_body * from_first(pointer ptr) noexcept;

	public:
		      value_type * get_first()          noexcept  { return m_body->elements; }
		const value_type * get_first()    const noexcept  { return m_body->elements; }
		      value_type * get_last()           noexcept  { auto * body = m_body.get(); return body->elements + body->size; }
		const value_type * get_last()     const noexcept  { auto * body = m_body.get(); return body->elements + body->size; }
		size_type    get_capacity() const noexcept  { return m_body->capacity; }
		size_type    get_size()     const noexcept  { return m_body->size; }

		const_pointers_tuple get_storage_pointers() const noexcept;
		pointers_tuple get_storage_pointers() noexcept;
		void set_storage_pointers(pointers_tuple pointers) noexcept;// { auto * body = m_body.get(); std::tie(body->first, body->last, body->end) = pointers; }

	public:
		auto allocate(allocator_type & alloc, size_type cap, const_pointer hint) -> std::pair<pointer, pointer>;
		void deallocate(allocator_type & alloc, pointer ptr, size_type cap) noexcept;
		auto allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newcap, const_pointer hint) -> std::pair<pointer, pointer>;

		auto checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::pair<pointer, pointer>;
		auto checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::pair<pointer, pointer>;

	private:
		void destruct() noexcept { }
		void copy_construct(const cow_vector_storage & other)     { m_body = other.m_body; }
		void move_construct(cow_vector_storage && other) noexcept { m_body = std::move(other.m_body); }
		void copy_assign(const cow_vector_storage & other)        { m_body = other.m_body; }
		void move_assign(cow_vector_storage && other) noexcept    { m_body = std::move(other.m_body); }

	public:
		static void set_allocator(const allocator_type & alloc) { /*static_cast<allocator_type &>(*this) = alloc;*/ };
		static auto get_allocator() noexcept { return allocator_type(); }
		//const allocator_type & get_allocator() const noexcept { return static_cast<const allocator_type &>(*this); }
		//      allocator_type & get_allocator()       noexcept { return static_cast<      allocator_type &>(*this); }

	public:
		cow_vector_storage() noexcept = default;
		cow_vector_storage(const allocator_type & alloc) noexcept /*: allocator_type(alloc)*/ {};

		cow_vector_storage(cow_vector_storage &&) noexcept = default;
		cow_vector_storage & operator =(cow_vector_storage &&) noexcept = default;
	};



	template <class Type, class Allocator>
	inline auto cow_vector_storage<Type, Allocator>::from_first(pointer ptr) noexcept -> heap_body *
	{
		auto uptr = reinterpret_cast<std::uintptr_t>(ptr);
		uptr -= sizeof(heap_body);
		return reinterpret_cast<heap_body *>(uptr);
	}

	template <class Type, class Allocator>
	inline auto cow_vector_storage<Type, Allocator>::get_storage_pointers() noexcept -> pointers_tuple
	{
		auto * body = m_body.get();
		auto first = body->elements;
		auto last = first + body->size;
		auto end = first + body->capacity;

		return {first, last, end};
	}

	template <class Type, class Allocator>
	inline auto cow_vector_storage<Type, Allocator>::get_storage_pointers() const noexcept -> const_pointers_tuple
	{
		auto * body = m_body.get();
		auto first = body->elements;
		auto last = first + body->size;
		auto end = first + body->capacity;

		return {first, last, end};
	}

	template <class Type, class Allocator>
	inline void cow_vector_storage<Type, Allocator>::set_storage_pointers(pointers_tuple pointers) noexcept
	{
		pointer first, last, end;
		std::tie(first, last, end) = pointers;

		auto * ptr = from_first(first);
		ptr->size = last - first;
		ptr->capacity = end - first;

		//m_body.reset(ptr);
		m_body.release();
		m_body.reset(ptr, ext::noaddref);
	}

	template <class Type, class Allocator>
	auto cow_vector_storage<Type, Allocator>::allocate(allocator_type & alloc, size_type cap, const_pointer hint) -> std::pair<pointer, pointer>
	{
		size_type realcap = cap * sizeof(Type) + sizeof(heap_body);

		using char_allocator_type   = typename allocator_traits::template rebind_alloc<char>;
		using char_allocator_traits = typename allocator_traits::template rebind_traits<char>;

		char_allocator_type char_alloc = alloc;

		auto * body_ptr = reinterpret_cast<heap_body *>(char_allocator_traits::allocate(char_alloc, realcap));
		auto * first = body_ptr->elements;
		auto * end   = first + cap;

		body_ptr->refs = 1;
		body_ptr->size = 0;
		body_ptr->capacity = cap;

		return {first, end};
	}

	template <class Type, class Allocator>
	void cow_vector_storage<Type, Allocator>::deallocate(allocator_type & alloc, pointer ptr, size_type cap) noexcept
	{
		size_type realcap = cap * sizeof(Type) + sizeof(heap_body);
		heap_body * body_ptr = from_first(ptr);

		using char_allocator_type   = typename allocator_traits::template rebind_alloc<char>;
		using char_allocator_traits = typename allocator_traits::template rebind_traits<char>;
		char_allocator_type char_alloc = alloc;

		char_allocator_traits::deallocate(char_alloc, reinterpret_cast<char *>(body_ptr), realcap);
	}

	template <class Type, class Allocator>
	auto cow_vector_storage<Type, Allocator>::allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newcap, const_pointer hint) -> std::pair<pointer, pointer>
	{
		using char_allocator_type   = typename allocator_traits::template rebind_alloc<char>;
		using char_allocator_traits = typename allocator_traits::template rebind_traits<char>;
		char_allocator_type char_alloc = alloc;

		auto cap = (curcap / 2 <= newcap / 3) ? newcap : curcap + curcap / 2;
		void * newptr;

		auto realcap = cap * sizeof(Type) + sizeof(heap_body);
		auto realnewcap = newcap * sizeof(Type)+ sizeof(heap_body);

		if (realcap < realnewcap or (newptr = char_allocator_traits::allocate(char_alloc, realcap)) == nullptr)
		{
			realcap = realnewcap, cap = newcap;
			newptr = char_allocator_traits::allocate(char_alloc, realcap);
		}

		auto * body_ptr = static_cast<heap_body *>(newptr);
		auto * first = body_ptr->elements;
		auto * end   = first + cap;

		body_ptr->refs = 1;
		body_ptr->size = 0;
		body_ptr->capacity = cap;

		return {first, end};
	}

	template <class Type, class Allocator>
	inline auto cow_vector_storage<Type, Allocator>::checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::pair<pointer, pointer>
	{
		vector_facade<cow_vector_storage>::check_newsize(newsize, sizeof(heap_body));
		return allocate_adjusted(alloc, curcap, newsize, hint);
	}

	template <class Type, class Allocator>
	inline auto cow_vector_storage<Type, Allocator>::checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::pair<pointer, pointer>
	{
		auto newsize = vector_facade<cow_vector_storage>::check_newsize(cursize, sizeof(heap_body));
		vector_facade<cow_vector_storage>::check_newsize(newsize, sizeof(heap_body));
		return allocate_adjusted(alloc, curcap, newsize, hint);
	}



	template <class Type>
	auto cow_heap_body<Type>::init_shared_null() -> ext::intrusive_cow_ptr<cow_heap_body<Type>>
	{
		auto * ptr = reinterpret_cast<cow_heap_body<Type> *>(&ms_storage);

		ptr->refs = 1;
		ptr->capacity = ptr->size = 0;

		return ext::intrusive_cow_ptr(ptr);
	}


	template <class Type> inline void intrusive_ptr_add_ref(cow_heap_body<Type> * ptr) noexcept   { ++ptr->refs; }
	template <class Type> inline void intrusive_ptr_release(cow_heap_body<Type> * ptr) noexcept;  //{ if (--ptr->refs == 0) delete ptr; }
	template <class Type> inline unsigned intrusive_ptr_use_count(const cow_heap_body<Type> * ptr) noexcept { return ptr->refs; }
	template <class Type> inline cow_heap_body<Type> * intrusive_ptr_default(const cow_heap_body<Type> * ptr) noexcept { cow_heap_body<Type>::ms_shared_null.addref(); return cow_heap_body<Type>::ms_shared_null.get(); }
	template <class Type> void intrusive_ptr_clone(const cow_heap_body<Type> * old_body, cow_heap_body<Type> * & dest);


	template <class Type> inline void intrusive_ptr_release(cow_heap_body<Type> * ptr) noexcept
	{
		if (--ptr->refs != 0) return;

		cow_heap_body<Type>::destruct(ptr);
	}

	template <class Type>
	void cow_heap_body<Type>::destruct(cow_heap_body * ptr)
	{
		auto * first = ptr->elements;
		auto * last  = first + ptr->size;

		using allocator_type = std::allocator<Type>;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using char_allocator_type   = typename allocator_traits::template rebind_alloc<char>;
		using char_allocator_traits = typename allocator_traits::template rebind_traits<char>;

		std::allocator<Type> alloc;
		char_allocator_type char_alloc = alloc;

		destroy(alloc, first, last);
		char_allocator_traits::deallocate(char_alloc, reinterpret_cast<char *>(ptr), ptr->capacity);
	}

	template <class Type>
	void intrusive_ptr_clone(const cow_heap_body<Type> * old_body, cow_heap_body<Type> * & dest)
	{
		using heap_body = cow_heap_body<Type>;
		using allocator_type = std::allocator<Type>;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using char_allocator_type   = typename allocator_traits::template rebind_alloc<char>;
		using char_allocator_traits = typename allocator_traits::template rebind_traits<char>;

		std::allocator<Type> alloc;
		char_allocator_type char_alloc = alloc;

		auto realcap = old_body->capacity * sizeof(Type) + sizeof(heap_body);
		auto * new_body = reinterpret_cast<heap_body *>(char_allocator_traits::allocate(char_alloc, realcap));
		new_body->refs = 1;

		auto old_first = old_body->elements;
		auto old_last  = old_first + old_body->size;

		new_body->size = old_body->size;
		new_body->capacity = old_body->capacity;
		uninitialized_copy(alloc, old_first, old_last, new_body->elements);

		dest = new_body;
	}
}
