#pragma once
#include <string>
#include <ostream>

///Содержит ascii case insensitive(aci) char_traits
///что описывает строку нечувствительную к регистру в рамках набора символов ascii(первые 127)
namespace ext
{
	struct aci_char_traits : std::char_traits<char>
	{
	private:
		///std::toupper(c, std::locale::classic())
		///достаточно медленен, как минимум на msvs 2010
		///поэтому используем ручную реализацию
		static char toupper(char c)
		{
			if (c >= 'a' && c <= 'z')
				return c + 'A' - 'a';
			else
				return c;
		}
	public:
		static bool eq(char c1, char c2)
		{
			return toupper(c1) == toupper(c2);
		}

		static bool lt(char c1, char c2)
		{
			return toupper(c1) < toupper(c2);
		}

		static int compare(const char * s1, const char * s2, size_t n)
		{
			while ( n-- )
			{
				//if (lt(*s1, *s2)) return -1;
				//if (lt(*s2, *s1)) return +1;
				char c1 = toupper(*s1);
				char c2 = toupper(*s2);
				if (c1 < c2) return -1;
				if (c2 < c1) return +1;
				
				++s1; ++s2;
			}
			return 0;
		}

		static const char * find(const char * s, std::size_t n, char a)
		{
			const char * end = s + n;
			for (; s < end; ++s)
			{
				if (eq(*s, a))
					return s;
			}
			
			return nullptr;
		}
	};

	typedef std::basic_string<char, aci_char_traits> aci_string;

	inline
	std::ostream & operator <<(std::ostream & os, aci_string const & str)
	{
		return os << str.c_str();
	}
}