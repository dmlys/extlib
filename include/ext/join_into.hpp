#pragma once

#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/as_literal.hpp>

namespace ext
{
	///конкатенирует строки в списке input соединяя разделителем sep и записывает результат в out
	///алгоритм - копия boost::algorihm::join с той лишь разницей что он не создает контейнер,
	///а пишет в заданный итератор
	///это позволяет использовать его с объектами не являющимися владельцами строк,
	///как например boost::iterator_range, boost::string_ref
	///
	///NOTE: если нужен join_if - используйте boost::adaptors::filtered
	template <class Range, class Separator, class OutIterator>
	OutIterator join_into(Range const & input, Separator const & sep, OutIterator out)
	{
		auto beg = boost::begin(input);
		auto end = boost::end(input);

		if (beg == end)
			return out;

		out = boost::copy(boost::as_literal(*beg), out);

		auto sepr = boost::as_literal(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			out = boost::copy(sepr, out);
			out = boost::copy(boost::as_literal(*it), out);
		}

		return out;
	}
}