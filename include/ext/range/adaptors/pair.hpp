#pragma once
#include <boost/range.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <ext/utility.hpp>

namespace ext
{
	namespace detail
	{
		template <class ForwardRange, std::size_t Idx>
		struct getted_range :
			boost::iterator_range<
				boost::transform_iterator<get_func<Idx>, typename boost::range_iterator<ForwardRange>::type>
			>
		{
		private:
			typedef boost::iterator_range<
				boost::transform_iterator<get_func<Idx>, typename boost::range_iterator<ForwardRange>::type>
			> base;
		public:
			typedef typename base::iterator iterator;

			getted_range(ForwardRange & rng)
				: base(iterator(boost::begin(rng)), iterator(iterator(boost::end(rng)))) {}
		};

		struct range_of_firsts {};
		struct range_of_seconds {};

		template <class ForwardRange>
		getted_range<ForwardRange, 0> operator | (ForwardRange & rng, range_of_firsts)
		{
			return getted_range<ForwardRange, 0>(rng);
		}

		template <class ForwardRange>
		getted_range<const ForwardRange, 0> operator | (ForwardRange const & rng, range_of_firsts)
		{
			return getted_range<const ForwardRange, 0>(rng);
		}

		template <class ForwardRange>
		getted_range<ForwardRange, 1> operator | (ForwardRange & rng, range_of_seconds)
		{
			return getted_range<ForwardRange, 1>(rng);
		}

		template <class ForwardRange>
		getted_range<const ForwardRange, 1> operator | (ForwardRange const & rng, range_of_seconds)
		{
			return getted_range<const ForwardRange, 1>(rng);
		}
	}

	const detail::range_of_firsts firsts;
	const detail::range_of_seconds seconds;
}