#pragma once
#include <iterator>

namespace ext
{
	///итератор, подобный ostream_iterator, но работающий напрямую со строкой
	///@Param String - тип строки std::string, std::wstring, ...
	template <class String>
	class append_iterator :
		public std::iterator<
			std::output_iterator_tag,
			void, void, void, void
		>
	{
		String * str;
		typedef typename String::value_type char_type;

		struct proxy
		{
			String * str;
			proxy(String * str_) : str(str_) {}

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

			proxy & operator =(char_type const * val)
			{
				str->append(val);
				return *this;
			}
		};

	public:
		append_iterator(String & str_) : str(&str_) {}

		append_iterator & operator ++() { return *this; }
		append_iterator & operator ++(int) { return *this; }
		proxy operator *() { return proxy(str); }
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
