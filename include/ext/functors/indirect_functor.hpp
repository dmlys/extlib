#pragma once
#include <type_traits> // for result_of
#include <boost/pointee.hpp>

namespace ext
{
	template <class Functor>
	class indirect_functor
	{
		Functor f;

	public:
		template <class... Args>
		inline
		typename std::result_of<Functor(typename boost::pointee<Args>::type& ...)>::type
			operator()(Args... args)
		{
			return f(*std::forward<Args>(args)...);
		}

		template <class... Args>
		inline
		typename std::result_of<Functor(typename boost::pointee<Args>::type& ...)>::type
			operator()(Args... args) const
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