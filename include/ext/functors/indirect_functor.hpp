#pragma once
#include <type_traits> // for result_of
#include <ext/type_traits.hpp>
#include <boost/pointee.hpp>

namespace ext
{
	template <class Functor>
	class indirect_functor
	{
		Functor f;

	public:
		template <class... Args>
		inline std::invoke_result_t<Functor, typename boost::pointee<ext::remove_cvref_t<Args>>::type ...>
			operator()(Args &&... args)
		{
			return f(*std::forward<Args>(args)...);
		}

		template <class... Args>
		inline std::invoke_result_t<Functor, typename boost::pointee<ext::remove_cvref_t<Args>>::type ...>
			operator()(Args &&... args) const
		{
			return f(*std::forward<Args>(args)...);
		}

		indirect_functor() : f(Functor {}) {}
		indirect_functor(Functor f) : f(std::move(f)) {}
	};

	template <class Functor>
	indirect_functor<Functor> make_indirect_functor(Functor f)
	{
		return f;
	}
}