#pragma once
#include <climits>    // for CHAR_BIT
#include <type_traits>
#include <iterator>   // for iterator_traits
#include <cassert>

///здесь содержаться вспомогательные функция для обработки utf-8 последовательностей

static_assert(CHAR_BIT == 8, "char is not 8 bit");

namespace ext
{
	namespace utf8_utils
	{
		const char Bom[] = "\xEF\xBB\xBF";
		const int BomSize = 3;

		//смотри описание кодирования http://en.wikipedia.org/wiki/UTF-8#Description
		//для ясности старший бит - 7, младший - 0
		//0xxx xxxx - ASCII символ
		//11xx xxxx - начало последовательности, длина которой будет определяться битами идущими после 6го
		//10xx xxxx - не первый символ в последовательности

		/// проверяет что заданный char есть начало последовательности
		inline bool is_seqbeg(char ch)
		{
			return (ch & 0x80U) == 0 || (ch & 0xC0U) == 0xC0U;
		}

		/// вычисляет длину последовательности по символу заголовку
		inline unsigned seqlen(char ch)
		{
			assert(is_seqbeg(ch));
			unsigned uci = static_cast<unsigned char>(ch);
			if (uci < 0x80) // ascii
				return 1;
			else if (uci < 0xE0) // 0b 110x xxxx - if less than 0b 1110 0000
				return 2;
			else if (uci < 0xF0) // 0b 1110 xxxx - if less than 0b 1111 0000
				return 3;
			else if (uci < 0xF8) // 0b 1111 0xxx - if less than 0b 1111 1000
				return 4;
			else
				return 4;

			//else if (uci < 0xFC) // 0b 1111 10xx - if less than 0b 1111 1100
			//	return 5;
			//else //if (uci < 0xFE) // 0b 1111 110x - if less than 0b 1111 1110
			//	return 6;
		}

		/// усекает заданную последовательность [beg; end)
		/// по последней полной utf-8 последовательности
		/// возвращает итератор конца усеченной последовательности(после последнего байта последовательности)
		///
		/// обработка идет с конца и никак не учитывает не полные последовательности в середине/начале
		template <class Iterator>
		Iterator rtrunc(Iterator beg, Iterator end)
		{
			static_assert(std::is_same<typename std::iterator_traits<Iterator>::value_type, char>::value,
			              "not a char");

			if (beg == end)
				return end;

			auto cur = end;
			--cur;
			
			while (cur != beg && !is_seqbeg(*cur))
				--cur;

			// нарушение этого assert'а возможно при некорректном utf-8, иначе же по *beg всегда должен быть seqbeg
			assert(is_seqbeg(*cur));
			return cur + seqlen(*cur) == end ? end : cur;
		}


		template <class Container>
		Container trunc_copy(Container const & cont, std::size_t newSize)
		{
			std::size_t curSize = cont.size();
			if (newSize >= curSize)
				return cont;

			auto beg = cont.begin();
			auto end = beg + newSize;

			end = rtrunc(beg, end);
			return {beg, end};
		}

		template <class Container>
		void trunc(Container & cont, std::size_t newSize)
		{
			std::size_t curSize = cont.size();
			if (newSize >= curSize)
				return;

			auto beg = cont.begin();
			auto end = beg + newSize;

			end = rtrunc(beg, end);
			cont.erase(end, cont.end());
		}
	}
}