#include <ext/itoa.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(itoa_test)
{
	const std::size_t sz = 30;
	char buffer[sz];
	char * res;

	using ext::unsafe_itoa;
	res = unsafe_itoa(30, buffer, sz, 10);
	BOOST_CHECK(strcmp(res, "30") == 0);

	res = unsafe_itoa(-30, buffer, sz, 10);
	BOOST_CHECK(strcmp(res, "-30") == 0);

	res = unsafe_itoa(0, buffer, sz, 10);
	BOOST_CHECK(strcmp(res, "0") == 0);

	res = unsafe_itoa(-1, buffer, sz, 10);
	BOOST_CHECK(strcmp(res, "-1") == 0);

	res = unsafe_itoa(INT_MAX, buffer, sz, 10);
	BOOST_CHECK(res == std::to_string(INT_MAX));

	res = unsafe_itoa(INT_MIN, buffer, sz, 10);
	BOOST_CHECK(res == std::to_string(INT_MIN));
}