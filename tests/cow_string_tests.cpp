#include <ext/strings/cow_string.hpp>
#include <boost/test/unit_test.hpp>
#include "string_tests.h"


BOOST_AUTO_TEST_CASE(cow_string_tests)
{
	typedef string_test_case<ext::cow_string> tests;
	tests::run_all_test();
}
