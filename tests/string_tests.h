#include <boost/test/unit_test.hpp>
#include <string>
#include <string_view>

template <class String>
struct string_test_case
{
	typedef String string_type;

public:
	static void assign_test();
	static void append_test();
	static void concat_tests();
	static void insert_tests();
	static void replace_tests();
	static void erase_tests();

	static void string_view_tests();

	static void find_tests();
	static void rfind_tests();
	static void find_first_of_tests();
	static void find_last_of_tests();

	static void run_all_test();
};

template <class String>
void string_test_case<String>::assign_test()
{
	string_type source;
	string_type str;

	source.assign("some text");
	BOOST_CHECK_EQUAL(source, "some text");

	str.assign(source, 3, 3);
	BOOST_CHECK_EQUAL(str, "e t");

	str.assign(source, 5);
	BOOST_CHECK_EQUAL(str, "text");

	auto first = str.data();
	str.assign(first, first + 2);
	BOOST_CHECK_EQUAL(str, "te");

	// overlap
	str.assign("some text");
	str.assign(str.data() + 1, 4);
	BOOST_CHECK_EQUAL(str, "ome ");

	str.assign("some text");
	str.assign(str, 5);
	BOOST_CHECK_EQUAL(str, "text");
}

template <class String>
void string_test_case<String>::append_test()
{
	string_type source = "some text for append tests";
	string_type str;
	
	str.append(source);
	BOOST_CHECK_EQUAL(str, source);

	str.clear();
	str = "123 ";
	str.append(source, 10, 3);
	BOOST_CHECK_EQUAL(str, "123 for");

	auto first = source.data();
	str.append(first, first + 4);
	BOOST_CHECK_EQUAL(str, "123 forsome");

	first = str.data();
	str.append(first + 7, first + 7 + 4);
	BOOST_CHECK_EQUAL(str, "123 forsomesome");

	first = str.data();
	str.append(first, first + 3);
	BOOST_CHECK_EQUAL(str, "123 forsomesome123");

	// overlap
	str.assign("some text 123");
	str.append(str, 4, 5);
	BOOST_CHECK_EQUAL(str, "some text 123 text");
	
	str.append(str.data() + 10, 8);
	BOOST_CHECK_EQUAL(str, "some text 123 text123 text");
}

template <class String>
void string_test_case<String>::concat_tests()
{
	string_type str = "first";
	string_type res = "prefix " +
		str +
		" " +
		"second";
	BOOST_CHECK_EQUAL(res, "prefix first second");

	res = "prefix " + string_type("first") + " second";
	BOOST_CHECK_EQUAL(res, "prefix first second");

	res.clear();
	res.shrink_to_fit();
	str = "some string test";
	res = "some string testsome string test";

	str = str + str;
	BOOST_CHECK_EQUAL(str, res);
}

template <class String>
void string_test_case<String>::insert_tests()
{
	string_type str = "some text";
	string_type toins = "instxt";

	str.insert(4, " " + toins);
	BOOST_CHECK_EQUAL(str, "some instxt text");

	str.insert(str.size(), str, 4, toins.size() + 1);
	BOOST_CHECK_EQUAL(str, "some instxt text instxt");
}

template <class String>
void string_test_case<String>::replace_tests()
{
	string_type str = "this is bad something";
	std::string vbad = "bad";        // std::string for different iterator type,
	std::string vgood = "verygood";  // so proper overload will be choosen

	auto pos = str.find(vbad.c_str());

	auto * ptr = str.data();
	str.replace(ptr + pos, ptr + pos + vbad.size(), vgood.begin(), vgood.end());
	BOOST_CHECK_EQUAL(str, "this is verygood something");

	ptr = str.data();
	str.replace(ptr + pos, ptr + pos + vgood.size(), vbad.begin(), vbad.end());
	BOOST_CHECK_EQUAL(str, "this is bad something");

	str = "this is bad something";
	str.shrink_to_fit();

	str.replace(pos, vbad.size(), vgood.c_str());
	BOOST_CHECK_EQUAL(str, "this is verygood something");

	str.replace(pos, vgood.size(), vbad.c_str());
	BOOST_CHECK_EQUAL(str, "this is bad something");

	/************************************************************************/
	/*             replace overlap self_type overload tests                 */
	/************************************************************************/
	// overlap, substring begins before hole
	str = "this is bad something"; // -> "bad something is bad something"
	str.replace(0, 4, str, 8);
	BOOST_CHECK_EQUAL(str, "bad something is bad something");

	// overlap, substring begins after hole
	str = "some txt";
	str.replace(4, 1, str, 0);
	BOOST_CHECK_EQUAL(str, "somesome txttxt");

	// overlap, substring begins inside hole
	str = "abcdef";
	str.replace(0, 2, str, 1, 3);
	BOOST_CHECK_EQUAL(str, "bcdcdef");

	// overlap before and over hole
	str = "abcdef";
	str.replace(0, 4, str, 0, 5);
	BOOST_CHECK_EQUAL(str, "abcdeef");

	str = "123";
	str.replace(str.size(), 0, str);
	BOOST_CHECK_EQUAL(str, "123123");

	/************************************************************************/
	/*             replace overlap char *    overload test                  */
	/************************************************************************/
	// overlap, shrink test
	str = "bad something is bad something";
	auto f1 = str.data() + str.rfind("bad something");
	auto l1 = f1 + std::strlen("bad something");
	str.replace(str.begin(), str.end(), f1, l1);
	BOOST_CHECK_EQUAL(str, "bad something");


	// overlap, substring begins before hole
	str = "this is bad something"; // -> "bad something is bad something"
	str.replace(str.begin() + 0, str.begin() + 4, str.begin() + 8, str.end());
	BOOST_CHECK_EQUAL(str, "bad something is bad something");

	// overlap, substring begins after hole
	str = "some txt";
	str.replace(str.begin() + 4, str.begin() + 4 + 1, str.begin() + 0, str.end());
	BOOST_CHECK_EQUAL(str, "somesome txttxt");

	// overlap, substring begins inside hole
	str = "abcdef";
	str.replace(str.begin() + 0, str.begin() + 2, str.begin() + 1, str.begin() + 1 + 3);
	BOOST_CHECK_EQUAL(str, "bcdcdef");

	// overlap before and over hole
	str = "abcdef";
	str.replace(str.begin() + 0, str.begin() + 4, str.begin() + 0, str.begin() + 5);
	BOOST_CHECK_EQUAL(str, "abcdeef");

}

template <class String>
void string_test_case<String>::erase_tests()
{
	string_type str = "some text";
	
	str.erase(0, 5);
	BOOST_CHECK_EQUAL(str, "text");
}


template <class String>
void string_test_case<String>::string_view_tests()
{
	using namespace std::literals;

	string_type str("test"sv);
	BOOST_CHECK_EQUAL(str, "test");

	str += " world"sv;
	BOOST_CHECK_EQUAL(str, "test world");

	str.replace(0, 5, ""sv);
	BOOST_CHECK_EQUAL(str, "world");

	str.replace(0, str.size(), "world hello"sv, 6, str.npos);
	BOOST_CHECK_EQUAL(str, "hello");
}

template <class String>
void string_test_case<String>::find_tests()
{
	string_type s = "some";

	size_t pos;
	pos = s.find('s');
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find("some");
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find(s);
	BOOST_CHECK_EQUAL(pos, 0);
}

template <class String>
void string_test_case<String>::rfind_tests()
{
	string_type s = "some";

	size_t pos;
	pos = s.rfind('s');
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.rfind("some");
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.rfind(s);
	BOOST_CHECK_EQUAL(pos, 0);
}

template <class String>
void string_test_case<String>::find_first_of_tests()
{
	string_type s = "some";
	size_t pos;

	pos = s.find_first_of("om");
	BOOST_CHECK_EQUAL(pos, 1);

	pos = s.find_first_of("s", 0);
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find_first_of("e", 3);
	BOOST_CHECK_EQUAL(pos, 3);

	pos = s.find_first_of(string_type("123"));
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_first_of(string_type("me"));
	BOOST_CHECK_EQUAL(pos, 2);
}

template <class String>
void string_test_case<String>::find_last_of_tests()
{
	string_type s = "some";
	size_t pos;

	pos = s.find_last_of("om");
	BOOST_CHECK_EQUAL(pos, 2);

	pos = s.find_last_of("s", 0);
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of("e", 3);
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of(string_type("123"));
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of(string_type("me"));
	BOOST_CHECK_EQUAL(pos, 3);
}

template <class String>
void string_test_case<String>::run_all_test()
{
	assign_test();
	append_test();
	concat_tests();
	insert_tests();
	erase_tests();
	replace_tests();
	string_view_tests();

	find_tests();
	rfind_tests();
	find_first_of_tests();
	find_last_of_tests();
}


