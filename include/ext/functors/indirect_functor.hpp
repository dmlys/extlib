#pragma once
#include <utility>

namespace ext
{
	template <class Functor>
	class indirect_functor
	{
		Functor f;

	public:
		template <class... Args>
		decltype(auto) operator()(Args &&... args)
		{
			return f(*std::forward<Args>(args)...);
		}

		template <class... Args>
		decltype(auto) operator()(Args &&... args) const
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
