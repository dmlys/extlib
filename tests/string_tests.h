#include <boost/test/unit_test.hpp>
#include <string>

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
	BOOST_CHECK(source == "some text");

	str.assign(source, 3, 3);
	BOOST_CHECK(str == "e t");

	str.assign(source, 5);
	BOOST_CHECK(str == "text");

	auto first = str.data();
	str.assign(first, first + 2);
	BOOST_CHECK(str == "te");
}

template <class String>
void string_test_case<String>::append_test()
{
	string_type source = "some text for append tests";
	string_type str;
	
	str.append(source);
	BOOST_CHECK(str == source);

	str.clear();
	str = "123 ";
	str.append(source, 10, 3);
	BOOST_CHECK(str == "123 for");

	auto first = source.data();
	str.append(first, first + 4);
	BOOST_CHECK(str == "123 forsome");

	first = str.data();
	str.append(first + 7, first + 7 + 4);
	BOOST_CHECK(str == "123 forsomesome");

	first = str.data();
	str.append(first, first + 3);
	BOOST_CHECK(str == "123 forsomesome123");
}

template <class String>
void string_test_case<String>::concat_tests()
{
	string_type str = "first";
	string_type res = "prefix " +
		str +
		" " +
		"second";
	BOOST_CHECK(res == "prefix first second");

	res = "prefix " + string_type("first") + " second";
	BOOST_CHECK(res == "prefix first second");

	res.clear();
	res.shrink_to_fit();
	str = "some string test";
	res = "some string testsome string test";

	str = str + str;
	BOOST_CHECK(str == res);
}

template <class String>
void string_test_case<String>::insert_tests()
{
	string_type str = "some text";
	string_type toins = "instxt";
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
	BOOST_CHECK(str == "this is verygood something");

	ptr = str.data();
	str.replace(ptr + pos, ptr + pos + vgood.size(), vbad.begin(), vbad.end());
	BOOST_CHECK(str == "this is bad something");

	str = "this is bad something";
	str.shrink_to_fit();

	str.replace(pos, vbad.c_str());
	BOOST_CHECK(str == "this is verygood something");

	str.replace(pos, vgood.c_str());
	BOOST_CHECK(str == "this is bad something");

}

template <class String>
void string_test_case<String>::find_tests()
{
	string_type s = "some";

	size_t pos;
	pos = s.find('s');
	BOOST_CHECK(pos == 0);

	pos = s.find("some");
	BOOST_CHECK(pos == 0);

	pos = s.find(s);
	BOOST_CHECK(pos == 0);
}

template <class String>
void string_test_case<String>::rfind_tests()
{
	string_type s = "some";

	size_t pos;
	pos = s.rfind('s');
	BOOST_CHECK(pos == 0);

	pos = s.rfind("some");
	BOOST_CHECK(pos == 0);

	pos = s.rfind(s);
	BOOST_CHECK(pos == 0);
}

template <class String>
void string_test_case<String>::find_first_of_tests()
{
	string_type s = "some";
	size_t pos;

	pos = s.find_first_of("om");
	BOOST_CHECK(pos == 1);

	pos = s.find_first_of("s", 0);
	BOOST_CHECK(pos == 0);

	pos = s.find_first_of("e", 3);
	BOOST_CHECK(pos == 3);

	pos = s.find_first_of(string_type("123"));
	BOOST_CHECK(pos == string_type::npos);

	pos = s.find_first_of(string_type("me"));
	BOOST_CHECK(pos == 2);
}

template <class String>
void string_test_case<String>::find_last_of_tests()
{
	string_type s = "some";
	size_t pos;

	pos = s.find_last_of("om");
	BOOST_CHECK(pos == 2);

	pos = s.find_last_of("s", 0);
	BOOST_CHECK(pos == string_type::npos);

	pos = s.find_last_of("e", 3);
	BOOST_CHECK(pos == string_type::npos);

	pos = s.find_last_of(string_type("123"));
	BOOST_CHECK(pos == string_type::npos);

	pos = s.find_last_of(string_type("me"));
	BOOST_CHECK(pos == 3);
}

template <class String>
void string_test_case<String>::run_all_test()
{
	assign_test();
	append_test();
	concat_tests();
	insert_tests();

	find_tests();
	rfind_tests();
	find_first_of_tests();
	find_last_of_tests();
}


