#pragma once
#include <ext/iterator/indirect_iterator.hpp>
#include <boost/range.hpp>

namespace ext
{
	namespace range_detail
	{
		struct outidrected_t {};

		template <class InputRange>
		struct indirected_range :
			boost::iterator_range<
			ext::indirect_iterator<typename boost::range_iterator<InputRange>::type>
			>
		{
		private:
			using base_type = boost::iterator_range<
				ext::indirect_iterator<typename boost::range_iterator<InputRange>::type>
			>;
		public:
			using iterator = typename base_type::iterator;

			indirected_range(InputRange & rng)
				: base_type(iterator(boost::begin(rng)), iterator(boost::end(rng))) {}
		};

		template <class InputRange>
		indirected_range<InputRange> operator | (InputRange & rng, outidrected_t)
		{
			return indirected_range<InputRange>(rng);
		}

		template <class InputRange>
		indirected_range<const InputRange> operator | (const InputRange & rng, outidrected_t)
		{
			return indirected_range<const InputRange>(rng);
		}
	}

	using range_detail::indirected_range;

	/// rng | indirected - makes indirected range, same as
	/// boost::make_iterator_range(ext::make_indirect_iterator(rng.begin(), ext::make_indirect_iterator(rng.end())
	const range_detail::outidrected_t indirected {};
}
