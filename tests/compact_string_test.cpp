#include <ext/strings/compact_string.hpp>
#include <boost/test/unit_test.hpp>
#include "string_tests.h"

typedef ext::basic_string_facade<
	ext::compact_string_base<sizeof(std::uintptr_t) * 2>,
	std::char_traits<char>
> double_compact_string;

typedef string_test_case<ext::compact_string> compact_test;
typedef string_test_case<double_compact_string> dcompact_test;

BOOST_AUTO_TEST_CASE(compact_string_test)
{
	compact_test::run_all_test();
	dcompact_test::run_all_test();
}
