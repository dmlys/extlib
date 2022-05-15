#pragma once
#include <type_traits>
#include <iterator>
#include <utility>  // for std::pair
#include <tuple>    // for std::tie

#include <ext/is_string.hpp>
#include <ext/range/range_traits.hpp>
#include <ext/range/str_view.hpp>

#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>

/// string interpolation functions similar to unix shells,
/// but allowed symbols are handled somewhat simplistically
/// ${var} forms allows any symbols
/// $var for allows only 0-9a-zA-Z_
///  $0, $_, $var
/// symbol escaping is supported \$var, $var\ cont, ${var\}}
/// No extended forms of expansion(bash like or any other) is supported (for example ${VAR:-12})
/// 
/// usage example:
///     std::map<string, string> dict = {{"name", "bond"}, {"date", "friday"}};
///     string res = ext::interpolate("my name is $name and this is test on $date", dict);
/// 
/// Dictionary - dictionary object:
///	  * can be std::map like - should have key_type, mapped_type typedefs
///     find begin, end methods
///   * invokable functor, taking std::basic_string as searchable argument,
///     returning std::optional<char container> as result
///
/// input range can be any boost compatible char range
/// supported char types: char/char16_t/char32_t/wchar_t.
/// result can be written into output iterator or output container.
///   for container ext::append function call should be valid

namespace ext
{
	/// interpolates range with given dictionary into dest iterator
	template <class Range, class Dictionary, class OutIter>
	std::enable_if_t<not ext::is_range_v<OutIter>, OutIter>
	interpolate(const Range & text, const Dictionary & dict, OutIter dest);

	/// interpolates range with given dictionary into dest container
	template <class Range, class Dictionary, class Container>
	std::enable_if_t<ext::is_container_v<Container>, void>
	interpolate(const Range & text, const Dictionary & dict, Container & outc);

	/// interpolates range with given dictionary and returning as std::string
	template <class Range, class Dictionary>
	auto interpolate(const Range & text, const Dictionary & dict) ->
		std::basic_string<ext::range_value_t<Range>>;
	
	/// interpolates range with given dictionary and returning as std::string
	template <class CharT, class CharTraits, class Allocator, class Dictionary>
	auto interpolate(const std::basic_string<CharT, CharTraits, Allocator> & text, const Dictionary & dict)
		-> std::basic_string<CharT, CharTraits, Allocator>;
	
	
	/************************************************************************/
	/*                   implementation                                     */
	/************************************************************************/
	namespace interpolate_internal
	{
		template <class Type>
		using get_key_type = typename Type::key_type;
		
		template <class Type>
		using get_mapped_type = typename Type::mapped_type;
		
		template <class Type>
		struct has_key_type : boost::mp11::mp_valid<get_key_type, Type> {};
		
		template <class Type>
		struct has_mapped_type : boost::mp11::mp_valid<get_mapped_type, Type> {};
		
		/// карта допустимых символов для переменных в не расширенном формате.
		/// это 0-9 a-z A-Z _
		extern const bool key_allowed_ascii_chars[128];

		struct key_start
		{
			template <class Char>
			inline bool operator()(Char ch) const { return ch == '$'; }
		};

		struct extended_key_stop
		{
			template <class Char>
			inline bool operator()(Char ch) const { return ch == '}'; }
		};
		
		struct key_stop
		{			
			template <class Char>
			std::enable_if_t<std::is_signed_v<Char>, bool>
			operator()(Char ch) const
			{
				return operator()(typename std::make_unsigned<Char>::type(ch));
			}

			template <class Char>
			std::enable_if_t<std::is_unsigned_v<Char>, bool>
			operator()(Char ch) const
			{
				return ch > 127 || !key_allowed_ascii_chars[ch];
			}
		};



		template <class Iterator, class OutputIterator, class StopFunc>
		std::pair<Iterator, OutputIterator> copy_escaped_until(Iterator first, Iterator last, OutputIterator out, StopFunc stop)
		{
			for (; first != last; ++first, ++out)
			{
				auto ch = *first;
				switch (ch)
				{
				// экранирующий символ, пока просто копируем следующий как есть.
				// при желании, обработку можно будет расширить
				case '\\':
					if (first == last) goto exit;
					*out = *++first;
					continue;

				default:
					if (stop(ch)) goto exit;
					*out = ch;
				}
			}

		exit: return {first, out};
		}
		
		
				
		/// implementation dictionary_find for std::map like classes
		template <class Dictionary, class Key>
		auto dictionary_find(const Dictionary & dict, const Key & key) -> const get_mapped_type<Dictionary> *
		{
			auto it = dict.find(key);
			return it != dict.end() ? &it->second : nullptr;
		}
		
		/// implementation dictionary_find for functions and alike
		template <class Dictionary, class Key>
		auto dictionary_find(const Dictionary & dict, const Key & key) -> std::invoke_result_t<const Dictionary, const Key>
		{
			return dict(key);
		}
		
		template <class Iterator, class OutIterator, class Dictionary>
		OutIterator interpolate_into_iterator(Iterator first, Iterator last, OutIterator out, const Dictionary & dict)
		{
			using char_type = typename std::iterator_traits<Iterator>::value_type;
			using key_type = boost::mp11::mp_eval_or<std::basic_string<char_type>, get_key_type, Dictionary>;
			
			key_type key;
			for (;;)
			{
				// ищем ключ, копируем строку
				std::tie(first, out) = copy_escaped_until(first, last, out, key_start());
				// не нашли символ ключа -> скопировали весь шаблон
				if (first == last)
					return out;
				
				++first;
				// после символа ключа - конец строки. копируем символ и выходим
				if (first == last)
				{
					*out++ = '$';
					return out;
				}

				bool extended;
				ext::clear(key);
				auto keyIt = std::back_inserter(key);
				// расширенный ключ
				if (*first == '{')
				{
					extended = true;
					std::tie(first, std::ignore) = copy_escaped_until(++first, last, keyIt, extended_key_stop());
					// ключ не закрыт
					if (first == last)
					{
						*out++ = '$'; *out++ = '{';
						out = std::copy(std::begin(key), std::end(key), out);
						return out;
					}

					++first; // съедаем '}'
				} // обычный ключ
				else
				{
					extended = false;
					std::tie(first, std::ignore) = copy_escaped_until(first, last, keyIt, key_stop());
					
					// $<space> - копируем символ и идем в начало алгоритма
					if (boost::empty(key)) {
						*out++ = '$';
						continue;
					}
				}
				
				/// выполняем подстановку
				auto val = ext::interpolate_internal::dictionary_find(dict, key);
				if (val)
					out = std::copy(boost::const_begin(*val), boost::const_end(*val), out);
				else
				{   // ключ 'отклонен' - копируем имя ключа как есть из шаблона
					*out++ = '$';
					if (extended) *out++ = '{';
					out = std::copy(boost::const_begin(key), boost::const_end(key), out);
					if (extended) *out++ = '}';
				}
			}
		}

		template <class Iterator, class Containter, class Dictionary>
		void interpolate_into_container(Iterator first, Iterator last, Containter & cont, const Dictionary & dict)
		{
			using char_type = typename std::iterator_traits<Iterator>::value_type;
			using key_type = boost::mp11::mp_eval_or<std::basic_string<char_type>, get_key_type, Dictionary>;

			key_type key;
			auto outit = std::back_inserter(cont);
			for (;;)
			{
				// ищем ключ, копируем строку
				std::tie(first, outit) = copy_escaped_until(first, last, outit, key_start());
				// не нашли символ ключа -> скопировали весь шаблон
				if (first == last)
					return;

				++first;
				// после символа ключа - конец строки. копируем символ и выходим
				if (first == last)
				{
					*outit++ = '$';
					return;
				}

				bool extended;
				ext::clear(key);
				auto keyIt = std::back_inserter(key);
				// расширенный ключ
				if (*first == '{')
				{
					extended = true;
					std::tie(first, std::ignore) = copy_escaped_until(++first, last, keyIt, extended_key_stop());
					// ключ не закрыт
					if (first == last)
					{
						*outit++ = '$'; *outit++ = '{';
						outit = std::copy(std::begin(key), std::end(key), outit);
						return;
					}

					++first; // съедаем '}'
				} // обычный ключ
				else
				{
					extended = false;
					std::tie(first, std::ignore) = copy_escaped_until(first, last, keyIt, key_stop());

					// $<space> - копируем символ и идем в начало алгоритма
					if (boost::empty(key)) {
						*outit++ = '$';
						continue;
					}
				}
				
				/// выполняем подстановку
				auto val = ext::interpolate_internal::dictionary_find(dict, key);
				if (val)
					ext::append(cont, boost::const_begin(*val), boost::const_end(*val));
				else
				{   // ключ 'отклонен' - копируем имя ключа как есть из шаблона
					*outit++ = '$';
					if (extended) *outit++ = '{';
					ext::append(cont, boost::const_begin(key), boost::const_end(key));
					if (extended) *outit++ = '}';
				}
			}
		}
	} // namespace InterpolateInternal

	/// interpolates range with given dictionary into dest iterator
	template <class Range, class Dictionary, class OutIter>
	inline std::enable_if_t<not ext::is_range_v<OutIter>, OutIter>
	interpolate(const Range & text, const Dictionary & dict, OutIter dest)
	{
		auto textlit = ext::str_view(text);
		return ext::interpolate_internal::interpolate_into_iterator
			(boost::const_begin(textlit), boost::const_end(textlit), dest, dict);
	}

	/// interpolates range with given dictionary into dest container
	template <class Range, class Dictionary, class Container>
	inline std::enable_if_t<ext::is_container_v<Container>, void>
	interpolate(const Range & text, const Dictionary & dict, Container & outc)
	{
		auto textlit = ext::str_view(text);
		ext::interpolate_internal::interpolate_into_container
			(boost::const_begin(textlit), boost::const_end(textlit), outc, dict);
	}

	/// interpolates range with given dictionary and returning as std::string
	template <class Range, class Dictionary>
	inline auto interpolate(const Range & text, const Dictionary & dict) ->
		std::basic_string<ext::range_value_t<Range>>
	{
		std::basic_string<typename boost::range_value<Range>::type> res;
		ext::interpolate(text, dict, res);
		return res;
	}
	
	/// interpolates range with given dictionary and returning as std::string
	template <class CharT, class CharTraits, class Allocator, class Dictionary>
	inline auto interpolate(const std::basic_string<CharT, CharTraits, Allocator> & text, const Dictionary & dict)
		-> std::basic_string<CharT, CharTraits, Allocator>
	{
		std::basic_string<CharT, CharTraits, Allocator> res;
		ext::interpolate(text, dict, res);
		return res;
	}
}
