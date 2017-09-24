#pragma once
#include <iterator>

namespace ext
{
	///итератор, подобный ostream_iterator, но работающий напрямую со строкой
	///@Param String - тип строки std::string, std::wstring, ...
	template <class String>
	class append_iterator
	{
	public:
		using string_type = String;
		using char_type = typename string_type::value_type;

		using iterator_category = std::output_iterator_tag;
		using value_type = void;
		using reference = void;
		using pointer = void;
		using difference_type = std::ptrdiff_t;

	public:
		string_type * str;


		struct proxy
		{
			string_type * str;
			proxy(string_type * str_) : str(str_) {}

			proxy & operator =(char_type ch)
			{
				str->push_back(ch);
				return *this;
			}

			template <class Type>
			proxy & operator =(Type && val)
			{
				using std::begin; using std::end;
				str->append(begin(val), end(val));
				return *this;
			}

			proxy & operator =(const char_type * val)
			{
				str->append(val);
				return *this;
			}
		};

	public:
		append_iterator & operator ++() { return *this; }
		append_iterator & operator ++(int) { return *this; }
		proxy operator *() { return proxy(str); }

	public:
		append_iterator(string_type & str_) : str(&str_) {}
	};

	template <class String>
	append_iterator<String> make_append_iterator(String & str)
	{
		return append_iterator<String>(str);
	}
}

#ifdef _MSC_VER
namespace std
{
	template <class String>
	struct _Is_checked_helper<ext::append_iterator<String>> :
		public std::true_type {};
}
#endif
