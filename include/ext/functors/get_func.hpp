#pragma once
#include <cstddef>
#include <utility>

namespace ext_detail_adl_helper
{
	using std::get;

	template <std::size_t Idx, class Type>
	auto get_impl(Type && val) -> decltype( get<Idx>(std::forward<Type>(val)) )
	{
		return get<Idx>(std::forward<Type>(val));
	}
}

namespace ext
{
	// may be move to its own header

	//template <std::size_t Idx, class Type>
	//auto get(Type && val) -> decltype( ext_detail_adl_helper::get_impl<Idx>(std::forward<Type>(val)) )
	//{
	//	return ext_detail_adl_helper::get_impl<Idx>(std::forward<Type>(val));
	//}

	///позволяет извлекать элемент по индексу, эквивалентно std::get<Idx>(val)
	///Пример использования: std::transform(vec.begin(), vec.end(), vecint.begin(), get_func<0>());
	template <std::size_t Idx>
	struct get_func
	{
		template <class Type>
		auto operator()(Type && val) const -> decltype( ext_detail_adl_helper::get_impl<Idx>(std::forward<Type>(val)) )
		{
			return ext_detail_adl_helper::get_impl<Idx>(std::forward<Type>(val));
		}
	};

	typedef get_func<0> first_el;
	typedef get_func<1> second_el;
}