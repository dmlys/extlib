#pragma once
#include <ext/iterator/zip_iterator.hpp>
#include <boost/range.hpp>

namespace ext
{
	template <class... Ranges>
	class combined_range : 
		public boost::iterator_range<
			ext::zip_iterator<typename boost::range_iterator<Ranges>::type...>
		>
	{
	public:
		using zip_iterator = ext::zip_iterator<typename boost::range_iterator<Ranges>::type...>;

	private:
		using self_type = combined_range;
		using base_type = boost::iterator_range<zip_iterator>;

	public:
		combined_range(zip_iterator first, zip_iterator last)
			: base_type(first, last) {}
	};

	template<typename... Ranges>
	auto combine(Ranges && ... rngs) -> combined_range<std::decay_t<Ranges>...>
	{
		using zip_iterator = typename combined_range<Ranges...>::zip_iterator;
		return {zip_iterator {boost::begin(rngs)...}, zip_iterator {boost::end(rngs)...}};
	}
}
