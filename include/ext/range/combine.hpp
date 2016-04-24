#pragma once
#include <ext/zip_iterator.hpp>
#include <boost/range.hpp>

namespace ext
{
	template <class... Iterators>
	class combined_range : public boost::iterator_range<ext::zip_iterator<Iterators...>>
	{
		typedef boost::iterator_range<ext::zip_iterator<Iterators...>> base_type;

	public:
		combined_range(ext::zip_iterator<Iterators...> first, ext::zip_iterator<Iterators...> last)
			: base_type(first, last) {}
	};

	template<typename... Ranges>
	auto combine(Ranges && ... rngs) ->
		combined_range<typename boost::range_iterator<Ranges>::type...>
	{
		typedef ext::zip_iterator<typename boost::range_iterator<Ranges>::type...> iterator;
		return {iterator {boost::begin(rngs)...}, iterator {boost::end(rngs)...}};
	}
}