#pragma once
#include <type_traits>
#include <boost/range.hpp>
#include <ext/range.hpp>
#include <ext/algorithm/tribool_compare.hpp>

///Содержит предикаты сравнения аналогичные стандартным, но оперирующие char_traits
///Позволяет сравнивать 2 любых char range с помощью метода char_traits::compare
///это может быть полезно когда есть массивы неких char массивов, std::vector, boost::iterator_range<char *>, etc
///и их нужно сравнить не побитово, а скажем case insensetive.
///Отличие от boost::is_iequal в том что работает с последовательностями, и параметризируется не локалью, а char_traits
namespace ext {
namespace ctpred
{
	namespace detail
	{
		template <class Type>
		struct TraitsOrStringImpl
		{
			typedef Type type;
		};

		template <class Char, class Traits, class Allocator>
		struct TraitsOrStringImpl<std::basic_string<Char, Traits, Allocator>>
		{
			typedef Traits type;
		};

		template <class Type>
		struct TraitsOrString : TraitsOrStringImpl<typename std::decay<Type>::type> {};

		template <class Traits, class Char>
		int compare(const Char * ptr1, std::size_t n1,
		            const Char * ptr2, std::size_t n2)
		{
			int res = Traits::compare(ptr1, ptr2, (std::min)(n1, n2));
			if (res != 0)
				return res;

			//strings equal, length determines result, compare n1 <=> n2;
			return tribool_compare(n1, n2);
		}

		/// возможно стоит вынести в отдельный файл и "опубликовать" под каким-нить именем, например, literal_size
		/// + улучшить реализацию
		template <class CharRange>
		inline std::size_t size(const CharRange & rng)
		{
			return boost::size(rng);
		}

		template <class Char>
		inline std::size_t size(const Char * cstr)
		{
			return std::char_traits<Char>::length(cstr);
		}
	}

	template <class StringOrTraits, class Char>
	inline int compare(const Char * ptr1, std::size_t n1,
	                   const Char * ptr2, std::size_t n2)
	{
		typedef typename detail::TraitsOrString<StringOrTraits>::type traits;
		static_assert(std::is_same<Char, typename traits::char_type>::value, "char_type mismatch");
		return detail::compare<traits>(ptr1, n1, ptr2, n2);
	}

	template <class StringOrTraits, class CharRange1, class CharRange2>
	inline int compare(const CharRange1 & rng1, const CharRange2 & rng2)
	{
		return compare<StringOrTraits>(ext::data(rng1), detail::size(rng1),
		                               ext::data(rng2), detail::size(rng2));
	}

	template <class Type>
	struct equal_to
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			auto l1 = detail::size(r1);
			auto l2 = detail::size(r2);
			return l1 == l2 && compare<Type>(ext::data(r1), l1, ext::data(r2), l2) == 0;
		}
	};

	template <class Type>
	struct not_equal_to
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			auto l1 = detail::size(r1);
			auto l2 = detail::size(r2);
			return l1 != l2 || compare<Type>(ext::data(r1), l1, ext::data(r2), l2) != 0;
		}
	};

	template <class Type>
	struct greater
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			return compare<Type>(r1, r2) > 0;
		}
	};

	template <class Type>
	struct less
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			return compare<Type>(r1, r2) < 0;
		}
	};

	template <class Type>
	struct greater_equal
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			return compare<Type>(r1, r2) >= 0;
		}
	};

	template <class Type>
	struct less_equal
	{
		typedef bool result_type;

		template <class R1, class R2>
		bool operator()(R1 const & r1, R2 const & r2) const
		{
			return compare<Type>(r1, r2) <= 0;
		}
	};
}}