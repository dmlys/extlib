#include <ext/functors/ctpred.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ctpred_tests)
{
	auto s1 = "Test";
	std::string s2 = "Test";
	std::string s3 = "test";

	ext::ctpred::equal_to<std::string> eq;
	ext::ctpred::not_equal_to<std::string> neq;
	ext::ctpred::less<std::string> less;

	BOOST_CHECK(eq(s1, s2));
	BOOST_CHECK(neq(s2, s3));
	BOOST_CHECK(less(s2, s3));
}