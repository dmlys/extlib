#pragma once
#include <ostream>
#include <string>
#include <ext/is_string.hpp>
#include <ext/range/range_traits.hpp>

namespace ext { namespace pretty_printers
{
	/// декораторы по умолчанию
	template <class CharType>
	struct default_decorators
	{
		static const CharType * prefix;
		static const CharType * delimeter;
		static const CharType * postfix;
	};

	template <> const char * default_decorators<char>::prefix = "[";
	template <> const char * default_decorators<char>::postfix = "]";
	template <> const char * default_decorators<char>::delimeter = ", ";

	template <> const wchar_t * default_decorators<wchar_t >::prefix = L"[";
	template <> const wchar_t * default_decorators<wchar_t>::postfix = L"]";
	template <> const wchar_t * default_decorators<wchar_t>::delimeter = L", ";

	//template <> const char16_t * default_decorators<char16_t>::prefix = "[";
	//template <> const char16_t * default_decorators<char16_t>::postfix = "]";
	//template <> const char16_t * default_decorators<char16_t>::delimeter = ", ";

	//template <> const char32_t * default_decorators<char32_t>::prefix = "[";
	//template <> const char32_t * default_decorators<char32_t>::postfix = "]";
	//template <> const char32_t * default_decorators<char32_t>::delimeter = ", ";
	

	/// выводит заданный Range в os в виде [<item1>, ..., <itemn>]
	template <class CharType, class Range, class Decorators = default_decorators<CharType>>
	void print(std::basic_ostream<CharType> & os, const Range & rng,
	           const Decorators & decorators = default_decorators<CharType> {})
	{
		auto it = boost::begin(rng);
		auto end = boost::end(rng);

		os << decorators.prefix;
		if (it != end)
			os << *it++;
		for (; it != end; ++it)
			os << decorators.delimeter << *it;
		os << decorators.postfix;
	}

	/// для operator <<
	/// можно настроить игнорирование некоторых типов
	/// специализируйте данный класс для типа, который уже имеет оператор вывода в поток
	/// 
	/// это может быть полезным для избежания конфликтов с уже определенным оператором вывода типа
	/// например std::string, который является коллекцией, но для него уже есть оператор вывода
	template <class Type>
	struct stream_ignore : std::false_type {};

	/// игнорим std::string
	template <class Char, class Traits, class Allocator>
	struct stream_ignore<std::basic_string<Char, Traits, Allocator>> : std::true_type {};

	/// должен ли operator << для типа Type попадать в набор перегрузок
	template <class Type>
	struct is_streamable :
		std::integral_constant<bool,
			!stream_ignore<Type>::value &&
			!is_string<Type>::value &&
			 is_range<Type>::value
		> {};

	/// перегрузка оператора << для вывода Range
	/// использовать следующим образом:
	/// {
	///    vector<int> vi;
	///    vector<string> vs;
	///    list<int> li;
	///    auto ii = {1, 2, 3};
	///    int arr[3] = {1, 2, 3};
	///
	///    using ext::pretty_printers::operator <<;
	///    cout << vi << endl;
	///    cout << vs << endl;
	///    cout << li << endl;
	///    cout << ii << endl;
	///    cout << arr << endl;
	/// }
	template <class CharType, class Range,
	          typename std::enable_if<is_streamable<Range>::value, int>::type = 0
	>
	std::basic_ostream<CharType> & operator <<(std::basic_ostream<CharType> & os, const Range & rng)
	{
		print(os, rng);
		return os;
	}
}}