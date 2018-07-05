#pragma once
#include <iterator>
#include <boost/range.hpp>

namespace ext
{
	namespace range_detail
	{
		struct moved_t {};

		template <class InputRange>
		struct moved_range :
			boost::iterator_range<
				std::move_iterator<typename boost::range_iterator<InputRange>::type>
			>
		{
		private:
			typedef boost::iterator_range<
				std::move_iterator<typename boost::range_iterator<InputRange>::type>
			> base_type;

		public:
			typedef typename base_type::iterator iterator;

			moved_range(InputRange & rng)
				: base_type(iterator(boost::begin(rng)), iterator(boost::end(rng))) {}
		};

		template <class InputRange>
		moved_range<InputRange> operator | (InputRange & rng, moved_t)
		{
			return moved_range<InputRange>(rng);
		}

		template <class InputRange>
		moved_range<const InputRange> operator | (const InputRange & rng, moved_t)
		{
			return moved_range<const InputRange>(rng);
		}
	}

	using range_detail::moved_range;

	/// rng | moved - makes moved range, same as
	/// boost::make_iterator_range(std::make_move_iterator(rng.begin(), std::make_move_iterator(rng.end())
	const range_detail::moved_t moved {};
}