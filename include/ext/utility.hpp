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