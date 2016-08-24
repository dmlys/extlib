#pragma once
#include <type_traits>
#include <utility>
#include <tuple>
#include <functional>
//this utility.h acts as common header, so include integer_sequence
#include <ext/integer_sequence.hpp>

namespace ext
{
	// as_const, as in C++17
	// and unconst

	template <class Type> constexpr inline std::add_const_t<Type> & as_const(Type & ref) noexcept { return ref; }
	template <class Type> constexpr inline std::add_const_t<Type> * as_const(Type * ptr) noexcept { return ptr; }
	template <class Type> constexpr inline void as_const(const Type && ref) = delete;

	template <class Type> constexpr inline std::remove_const_t<Type> & unconst(const Type & ref) noexcept { return const_cast<std::remove_const_t<Type> &>(ref); }
	template <class Type> constexpr inline std::remove_const_t<Type> * unconst(const Type * ptr) noexcept { return const_cast<std::remove_const_t<Type> *>(ptr); }


	namespace detail
	{
		template <class Type>
		struct is_reference_wrapper : std::false_type {};

		template <class Type>
		struct is_reference_wrapper<std::reference_wrapper<Type>> : std::true_type {};

		/// pointer to member function + object
		template <class Base, class FuncType, class Derived, class ... Args>
		auto INVOKE(FuncType Base::*pmf, Derived && ref, Args && ... args)
			noexcept(noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value && 
				std::is_base_of<Base, std::decay_t<Derived>>::value,
				decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))
			>
		{
			return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member function + std::reference_wrapper of object
		template <class Base, class FuncType, class RefWrap, class ... Args>
		auto INVOKE(FuncType Base::*pmf, RefWrap && ref, Args && ... args)
			noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value &&
				is_reference_wrapper<std::decay_t<RefWrap>>::value,
				decltype((ref.get().*pmf)(std::forward<Args>(args)...))
			>
		{
			  return (ref.get().*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member function + pointer to object
		template <class Base, class FuncType, class Pointer, class ... Args>
		auto INVOKE(FuncType Base::*pmf, Pointer && ptr, Args && ... args)
			noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value && 
				is_reference_wrapper<std::decay_t<Pointer>>::value &&
				!std::is_base_of<Base, std::decay_t<Pointer>>::value,
				decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))
			>
		{
			return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member field + object
		template <class Base, class Type, class Derived>
		auto INVOKE(Type Base::*pmd, Derived && ref)
			noexcept(noexcept(std::forward<Derived>(ref).*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value &&
				std::is_base_of<Base, std::decay_t<Derived>>::value,
				decltype(std::forward<Derived>(ref).*pmd)
			>
		{
			return std::forward<Derived>(ref).*pmd;
		}

		/// pointer to member field + std::reference_wrapper of object
		template <class Base, class Type, class RefWrap>
		auto INVOKE(Type Base::*pmd, RefWrap && ref)
			noexcept(noexcept(ref.get().*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value && is_reference_wrapper<std::decay_t<RefWrap>>::value,
				decltype(ref.get().*pmd)
			>
		{
			return ref.get().*pmd;
		}

		/// pointer to member field + object pointer
		template <class Base, class Type, class Pointer>
		auto INVOKE(Type Base::*pmd, Pointer && ptr)
			noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value && 
				!is_reference_wrapper<std::decay_t<Pointer>>::value &&
				!std::is_base_of<Base, std::decay_t<Pointer>>::value,
				decltype((*std::forward<Pointer>(ptr)).*pmd)>
		{
			return (*std::forward<Pointer>(ptr)).*pmd;
		}

		/// functor is not a pointer to member
		template <class Functor, class ... Args>
		auto INVOKE(Functor && f, Args && ... args)
			noexcept(noexcept(std::forward<Functor>(f)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				!std::is_member_pointer<std::decay_t<Functor>>::value,
				decltype(std::forward<Functor>(f)(std::forward<Args>(args)...))
			>
		{
			return std::forward<Functor>(f)(std::forward<Args>(args)...);
		}
	}

	template <class Functor, class ... Args>
	auto invoke(Functor && f, Args && ... args)
		noexcept(noexcept(detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...)))
		-> decltype(detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...))
	{
		return detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...);
	}


	template <class Functor, class Tuple, std::size_t... I>
	constexpr decltype(auto) apply_impl(Functor && f, Tuple && t, std::index_sequence<I...>)
	{
		return ext::invoke(std::forward<Functor>(f), std::get<I>(std::forward<Tuple>(t))...);
	}

	template <class Functor, class Tuple>
	constexpr decltype(auto) apply(Functor && f, Tuple && t)
	{
		return apply_impl(std::forward<Functor>(f), std::forward<Tuple>(t),
		                  std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value> {});
	}


	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его с помощью параметров args
	/// полезно когда карта использует типы не default constructible
	template <class Map, class Key, class Arg>
	typename Map::mapped_type & accuqire_from_map(Map & map, const Key & key, Arg && arg)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto res = map.emplace(key, std::forward<Arg>(arg));
			return res.first->second;
		}
	}

	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его с помощью параметров args
	/// полезно когда карта использует типы не default constructible
	template <class Map, class Key, class ... Args>
	typename Map::mapped_type & accuqire_from_map(Map & map, const Key & key, Args && ... args)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto res = map.emplace(std::piecewise_construct,
			                       std::forward_as_tuple(key),
			                       std::forward_as_tuple(args...));
			
			return res.first->second;
		}
	}

	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его и инициализирует с помощью параметров args
	/// тип должен быть default constructable и быть способным инициализироваться выражением {..., ...}
	/// например простая структура:
	/// struct test {
	///    string s; int i; vector<int> vi;
	/// };
	template <class Map, class Key, class ... Args>
	typename Map::mapped_type & accuqire_init_from_map(Map & map, const Key & key, Args && ... args)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto & val = map[key];
			return val = {std::forward<Args>(args)...};
		}
	}

	template <class Map, class Key>
	typename Map::mapped_type * find_ptr(Map & map, const Key & key)
	{
		auto it = map.find(key);
		return it != map.end() ? &it->second : nullptr;
	}

	template <class Map, class Key>
	const typename Map::mapped_type * find_ptr(const Map & map, const Key & key)
	{
		auto it = map.find(key);
		return it != map.end() ? &it->second : nullptr;
	}
}