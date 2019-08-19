#pragma once
#include <type_traits>
#include <utility>
#include <memory>

namespace ext::container
{
	template <class storage_type>
	class vector_facade;


	template <class storage_type>
	class vector_storage_traits
	{
	public:
		using allocator_type = typename storage_type::allocator_type;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using value_type     = typename storage_type::value_type;
		using size_type      = typename storage_type::size_type;

		using const_pointer  = const value_type *;
		using       pointer  =       value_type *;


	public:
		template <class storage>
		struct have_allocate
		{
			template <class Storage, class = decltype(std::declval<Storage &>().allocate(std::declval<allocator_type &>(), std::declval<size_type>(), std::declval<const_pointer>()))>
			static auto test(int i) -> std::true_type;
			template <class Storage>
			static auto test(...) -> std::false_type;

			using type = decltype(test<storage>(0));
			static constexpr auto value = type::value;
		};

		template <class storage>
		struct have_deallocate
		{
			template <class Storage, class = decltype(std::declval<Storage &>().deallocate(std::declval<allocator_type &>(), std::declval<pointer>(), std::declval<size_type>()))>
			static std::true_type test(int i);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_allocate_adjusted
		{
			template <class Storage, class = decltype(std::declval<Storage &>().allocate_adjusted(std::declval<allocator_type &>(), std::declval<size_type>(), std::declval<size_type>(), std::declval<const_pointer>()))>
			static std::true_type test(int i);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_checked_allocate_adjusted_1
		{
			template <class Storage, class = decltype(std::declval<Storage &>().checked_allocate_adjusted(std::declval<allocator_type &>(), std::declval<size_type>(), std::declval<size_type>(), std::declval<const_pointer>()))>
			static std::true_type test(int i);
			template <class Storage>
			static std::false_type test(...);

			using type = decltype(test<storage>(0));
			static constexpr auto value = type::value;
		};

		template <class storage>
		struct have_checked_allocate_adjusted_2
		{
			template <class Storage, class = decltype(std::declval<Storage &>().checked_allocate_adjusted(std::declval<allocator_type &>(), std::declval<size_type>(), std::declval<size_type>(), std::declval<size_type>(), std::declval<const_pointer>()))>
			static std::true_type test(int i);
			template <class Storage>
			static std::false_type test(...);

			using type = decltype(test<storage>(0));
			static constexpr auto value = type::value;
		};

		template <class storage>
		struct have_destruct
		{
			template <class Storage, class = decltype(std::declval<Storage &>().destruct())>
			static std::true_type test(int);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_copy_construct
		{
			template <class Storage, class = decltype(Storage::copy_construct(std::declval<const Storage &>()), std::declval<const allocator_type &>())>
			static std::true_type test(int);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_move_construct
		{
			template <class Storage, class = decltype(Storage::move_construct(std::declval<Storage>()), std::declval<const allocator_type &>())>
			static std::true_type test(int);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_copy_assign
		{
			template <class Storage, class = decltype(std::declval<Storage &>().copy_assign(std::declval<const Storage &>()))>
			static std::true_type test(int);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

		template <class storage>
		struct have_move_assign
		{
			template <class Storage, class = decltype(std::declval<Storage &>().move_assign(std::declval<Storage>()))>
			static std::true_type test(int);
			template <class Storage>
			static std::false_type test(...);

			static constexpr auto value = decltype(test<storage>(0))::value;
		};

	public:
		template <class Storage>
		static auto allocate(Storage & st, allocator_type & alloc, size_type cap, const_pointer hint) -> std::enable_if_t<    have_allocate<Storage>::value, std::pair<pointer, pointer>>
		{ return st.allocate(alloc, cap, hint); }

		template <class Storage>
		static auto allocate(Storage & st, allocator_type & alloc, size_type cap, const_pointer hint) -> std::enable_if_t<not have_allocate<Storage>::value, std::pair<pointer, pointer>>
		{ return vector_facade<Storage>::vector_cast(st).default_allocate(alloc, cap, hint); }

		template <class Storage>
		static auto deallocate(Storage & st, allocator_type & alloc, pointer ptr, size_type cap) noexcept -> std::enable_if_t<    have_deallocate<Storage>::value>
		{ return st.deallocate(alloc, ptr, cap); }

		template <class Storage>
		static auto deallocate(Storage & st, allocator_type & alloc, pointer ptr, size_type cap) noexcept -> std::enable_if_t<not have_deallocate<Storage>::value>
		{ return vector_facade<Storage>::vector_cast(st).default_deallocate(alloc, ptr, cap); }

		template <class Storage>
		static auto allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::enable_if_t<    have_allocate_adjusted<Storage>::value, std::pair<pointer, pointer>>
		{ return st.allocate_adjusted(alloc, curcap, newsize, hint); }

		template <class Storage>
		static auto allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::enable_if_t<not have_allocate_adjusted<Storage>::value, std::pair<pointer, pointer>>
		{ return vector_facade<Storage>::vector_cast(st).default_allocate_adjusted(alloc, curcap, newsize, hint); }

		template <class Storage>
		static auto checked_allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::enable_if_t<    have_checked_allocate_adjusted_1<Storage>::value, std::pair<pointer, pointer>>
		{ return st.checked_allocate_adjusted(alloc, curcap, newsize, hint); }

		template <class Storage>
		static auto checked_allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::enable_if_t<not have_checked_allocate_adjusted_1<Storage>::value, std::pair<pointer, pointer>>
		{ return vector_facade<Storage>::vector_cast(st).default_checked_allocate_adjusted(alloc, curcap, newsize, hint); }

		template <class Storage>
		static auto checked_allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::enable_if_t<    have_checked_allocate_adjusted_2<Storage>::value, std::pair<pointer, pointer>>
		{ return st.checked_allocate_adjusted(alloc, curcap, cursize, increment, hint); }

		template <class Storage>
		static auto checked_allocate_adjusted(Storage & st, allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::enable_if_t<not have_checked_allocate_adjusted_2<Storage>::value, std::pair<pointer, pointer>>
		{ return vector_facade<Storage>::vector_cast(st).default_checked_allocate_adjusted(alloc, curcap, cursize, increment, hint); }

		template <class Storage>
		static auto destruct(Storage & st) noexcept -> std::enable_if_t<    have_destruct<Storage>::value>
		{ return st.destruct(); }

		template <class Storage>
		static auto destruct(Storage & st) noexcept -> std::enable_if_t<not have_destruct<Storage>::value>
		{ return vector_facade<Storage>::vector_cast(st).destruct(); }

		template <class Storage>
		static auto copy_construct(const Storage & other, const allocator_type & alloc) -> std::enable_if_t<    have_copy_construct<Storage>::value, storage_type>
		{ return Storage::copy_construct(other, alloc); }

		template <class Storage>
		static auto copy_construct(const Storage & other, const allocator_type & alloc) -> std::enable_if_t<not have_copy_construct<Storage>::value, storage_type>
		{ return vector_facade<Storage>::copy_construct(other, alloc); }

		template <class Storage>
		static auto move_construct(Storage && other, const allocator_type & alloc) noexcept -> std::enable_if_t<    have_move_construct<Storage>::value, storage_type>
		{ return Storage::move_construct(std::move(other), alloc); }

		template <class Storage>
		static auto move_construct(Storage && right, const allocator_type & alloc) noexcept -> std::enable_if_t<not have_move_construct<Storage>::value, storage_type>
		{ return vector_facade<Storage>::move_construct(std::move(right), alloc); }

		template <class Storage>
		static auto copy_assign(Storage & left, const Storage & right) -> std::enable_if_t<    have_copy_assign<Storage>::value>
		{ return left.copy_assign(right); }

		template <class Storage>
		static auto copy_assign(Storage & left, const Storage & right) -> std::enable_if_t<not have_copy_assign<Storage>::value>
		{ return vector_facade<Storage>::vector_cast(left).copy_assign(vector_facade<Storage>::vector_cast(right)); }

		template <class Storage>
		static auto move_assign(Storage & left, Storage && right) noexcept -> std::enable_if_t<    have_move_assign<Storage>::value>
		{ return left.move_assign(std::move(right)); }

		template <class Storage>
		static auto move_assign(Storage & left, Storage && right) noexcept -> std::enable_if_t<not have_move_assign<Storage>::value>
		{ return vector_facade<Storage>::vector_cast(left).move_assign(std::move(vector_facade<Storage>::vector_cast(right))); }
	};
}
