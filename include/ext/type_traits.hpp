#pragma once
#include <type_traits>

namespace ext
{
	namespace detail
	{
		/// some useful stuff for any type_traits

		/// Walter E. Brown trick
		template <class ...>
		struct voider
		{
			typedef void type;
		};

		template <class... Pack>
		using void_t = typename voider<Pack...>::type;


		struct Yes { char unused[1]; };
		struct No { char unused[2]; };
		static_assert(sizeof(Yes) != sizeof(No), "");
	}

}