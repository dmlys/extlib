#pragma once
#include <ext/iterator/outdirect_iterator.hpp>
#include <boost/range.hpp>

namespace ext
{
	namespace range_detail
	{
		struct outidrected_t {};

		template <class InputRange>
		struct outdirected_range :
			boost::iterator_range<
				ext::outdirect_iterator<typename boost::range_iterator<InputRange>::type>
			>
		{
		private:
			using base_type = boost::iterator_range<
				ext::outdirect_iterator<typename boost::range_iterator<InputRange>::type>
			>;
		public:
			using iterator = typename base_type::iterator;

			outdirected_range(InputRange & rng)
				: base_type(iterator(boost::begin(rng)), iterator(boost::end(rng))) {}
		};

		template <class InputRange>
		outdirected_range<InputRange> operator | (InputRange & rng, outidrected_t)
		{
			return outdirected_range<InputRange>(rng);
		}

		template <class InputRange>
		outdirected_range<const InputRange> operator | (const InputRange & rng, outidrected_t)
		{
			return outdirected_range<const InputRange>(rng);
		}
	}

	using range_detail::outdirected_range;

	/// rng | outdirected - makes outdirected range, same as
	/// boost::make_iterator_range(ext::make_outdirect_iterator(rng.begin(), ext::make_outdirect_iterator(rng.end())
	const range_detail::outidrected_t outdirected {};
}
