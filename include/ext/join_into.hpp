#pragma once
#include <ext/type_traits.hpp>
#include <ext/range/range_traits.hpp>
#include <ext/range/as_literal.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

namespace ext
{
	/// конкатенирует строки в списке input соединяя разделителем sep и записывает результат в out
	/// алгоритм - копия boost::algorihm::join с той лишь разницей что он не создает контейнер,
	/// а пишет в заданный итератор
	/// это позволяет использовать его с объектами не являющимися владельцами строк,
	/// как например boost::iterator_range, boost::string_ref
	/// 
	/// NOTE: если нужен join_if - используйте boost::adaptors::filtered
	template <class Range, class Separator, class OutIterator>
	auto join_into(const Range & input, const Separator & sep, OutIterator out)
		-> std::enable_if_t<ext::is_iterator<OutIterator>::value, OutIterator>
	{
		auto beg = boost::begin(input);
		auto end = boost::end(input);
		if (beg == end) return out;

		out = boost::copy(boost::as_literal(*beg), out);

		auto sepr = boost::as_literal(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			out = boost::copy(sepr, out);
			out = boost::copy(ext::as_literal(*it), out);
		}

		return out;
	}

	template <class Range, class Separator, class OutRange>
	auto join_into(const Range & input, const Separator & sep, OutRange & out)
		-> std::enable_if_t<ext::is_range<OutRange>::value>
	{
		auto beg = boost::begin(input);
		auto end = boost::end(input);
		if (beg == end) return;

		out = boost::push_back(out, ext::as_literal(*beg));

		auto sepr = boost::as_literal(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			out = boost::push_back(out, sepr);
			out = boost::push_back(out, ext::as_literal(*it));
		}
	}

	template <class Range, class Separator>
	auto join(const Range & input, const Separator & sep)
	{
		using value_type = typename boost::range_value<Range>::type;
		using char_type = typename boost::range_value<value_type>::type;
		using string = std::basic_string<char_type>;

		string out;
		join_into(input, sep, out);
		return out;
	}
}
