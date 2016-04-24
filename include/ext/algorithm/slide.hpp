#pragma once
#include <type_traits>
#include <algorithm>

namespace ext
{
	/// перемещает [first; last) на позицию pos с помощью std::rotate:
	/// если pos < first сразу после pos
	/// если pos > last перед pos
	/// возвращает пару итераторов указывающих на перемещенный диапазон
	///
	/// precondition: first < last < pos or pos < first < last
	/// postcondition [first; last) rotated to [pos; pos + (last - first)) if pos < first
	///               [first; last) rotated to [pos - (last - first); pos) if pos > last
	/// идея Sean Parent http://channel9.msdn.com/Events/GoingNative/2013/Cpp-Seasoning
	/// gather есть в 'boost/algorithm/gather.hpp
	template <class RandomAccessIterator>
	std::pair<RandomAccessIterator, RandomAccessIterator>
		slide(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator pos)
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::iterator_category iter_cat;
		static_assert(std::is_convertible<iter_cat, std::random_access_iterator_tag>::value, "not a random access iterator");

		if (pos < first) return {pos, std::rotate(pos, first, last)};
		if (last < pos)  return {std::rotate(first, last, pos), pos};
		return {first, last};
	}
}