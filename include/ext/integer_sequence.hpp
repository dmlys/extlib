#pragma once
#include <cstddef>
#include <boost/predef.h>
#include <boost/config.hpp>

/// реализация С++14 integer_sequence
namespace ext
{
	template <class T, T... Ints>
	class integer_sequence
	{
	public:
		typedef T value_type;
		static BOOST_CONSTEXPR std::size_t size() { return sizeof...(Ints); }
	};

	template <std::size_t... Ints>
	using index_sequence = integer_sequence<std::size_t, Ints...>;

	namespace integer_sequence_detail
	{
		/// implementation idea taken from:
		/// http://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence
		/// thanks to Xeo
		template<class T, class IntegerSeq1, class IntegerSeq2>
		struct concat;

		template <class T, T... seq1, T... seq2>
		struct concat<T, ext::integer_sequence<T, seq1...>, ext::integer_sequence<T, seq2...>>
		{
			typedef ext::integer_sequence<T, seq1..., (sizeof...(seq1) + seq2)...> type;
		};
		
		template <class T, T N, std::size_t HelpVal>
		struct generate_sequence;

		template <class T, T N, std::size_t HelpVal>
		struct generate_sequence
		{
			typedef typename concat<T,
				typename generate_sequence<T, N / 2, HelpVal / 2>::type,
				typename generate_sequence<T, N - N / 2, HelpVal - HelpVal / 2>::type
			>::type type;
		};
		
		template <class T, T N>
		struct generate_sequence<T, N, 0>
		{
			typedef ext::integer_sequence<T> type;
		};

		template <class T, T N>
		struct generate_sequence<T, N, 1>
		{
			typedef ext::integer_sequence<T, 0> type;
		};
	}

	/// helper to workaround msvc 2013 internal compiler error
	template <class T, T N>
	struct __make_integer_sequence
	{
		typedef typename integer_sequence_detail::generate_sequence<T, N, N>::type type;
	};

	/// helper to workaround msvc 2013 internal compiler error
	template <std::size_t N>
	struct __make_index_sequence
	{
		typedef typename integer_sequence_detail::generate_sequence<std::size_t, N, N>::type type;
	};

	/// helper to workaround msvc 2013 internal compiler error
	template <class... Types>
	struct __index_sequence_for
	{
		typedef typename __make_index_sequence<sizeof...(Types)>::type type;
	};

	template <class T, T N>
	using make_integer_sequence = typename __make_integer_sequence<T, N>::type;

	template <std::size_t N>
	using make_index_sequence = typename __make_index_sequence<N>::type;

	template<class... Types>
	using index_sequence_for = typename __index_sequence_for<Types...>::type;
}
