#pragma once
#include <boost/range.hpp>
#include <boost/locale/collator.hpp>
#include <ext/range/range_traits.hpp>

namespace ext
{
	namespace lcpred
	{
		struct is_equal
		{
			is_equal(std::locale const & loc_ = std::locale(),
			         boost::locale::collator_base::level_type level_ = boost::locale::collator_base::identical)
				: loc(loc_), level(level_) {}

			typedef bool result_type;
			template <class R1, class R2>
			result_type operator()(const R1 & r1, const R2 & r2) const
			{
				typedef typename boost::range_value<R1>::type char_type;
				typedef boost::locale::collator<char_type> Comp;
				return std::use_facet<Comp>(loc).
					compare(level, data(r1), data(r1) + boost::size(r1),
					               data(r2), data(r2) + boost::size(r2)) == 0;
			}

		private:
			std::locale loc;
			boost::locale::collator_base::level_type level;
		};

		struct is_iequal : is_equal
		{
			is_iequal(const std::locale & loc_ = std::locale(),
			          boost::locale::collator_base::level_type level_ = boost::locale::collator_base::primary)
				: is_equal(loc_, level_) {}
		};


		struct is_less
		{
			is_less(std::locale const & loc_ = std::locale(),
			        boost::locale::collator_base::level_type level_ = boost::locale::collator_base::identical)
				: loc(loc_), level(level_) {}

			typedef bool result_type;
			template <class R1, class R2>
			result_type operator()(const R1 & r1, const R2 & r2) const
			{
				typedef typename boost::range_value<R1>::type char_type;
				typedef boost::locale::collator<char_type> Comp;
				return std::use_facet<Comp>(loc).
					compare(level, data(r1), data(r1) + boost::size(r1),
					               data(r2), data(r2) + boost::size(r2)) < 0;
			}

		private:
			std::locale loc;
			boost::locale::collator_base::level_type level;
		};

		struct is_iless : is_less
		{
			is_iless(const std::locale & loc_ = std::locale(),
			         boost::locale::collator_base::level_type level_ = boost::locale::collator_base::primary)
				: is_less(loc_, level_) {}
		};
	}

}