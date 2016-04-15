#include <ext/strings/compact_string.hpp>
#include <boost/test/unit_test.hpp>
#include "string_tests.h"

typedef string_test_case<ext::compact_string> compact_test;

BOOST_AUTO_TEST_CASE(compact_string_test)
{
	compact_test::run_all_test();
}