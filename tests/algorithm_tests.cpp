#include <ext/algorithm.hpp>
#include <ext/functors/ctpred.hpp>
#include <ext/strings/aci_string.hpp>
#include <set>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(tribool_compare_test)
{
	using namespace std;
	using ext::tribool_compare;
	
	BOOST_CHECK(tribool_compare(1, 2) < 0);
	BOOST_CHECK(tribool_compare(2, 1) > 0);
	BOOST_CHECK(tribool_compare(10, 10) == 0);

	BOOST_CHECK(tribool_compare(1L, 2L) < 0);
	BOOST_CHECK(tribool_compare(2L, 1L) > 0);
	BOOST_CHECK(tribool_compare(10L, 10L) == 0);
	
	BOOST_CHECK(tribool_compare(1U, 2U) < 0);
	BOOST_CHECK(tribool_compare(2U, 1U) > 0);
	BOOST_CHECK(tribool_compare(10U, 10U) == 0);

	BOOST_CHECK(tribool_compare(1.0, 2.0) < 0);
	BOOST_CHECK(tribool_compare(2.0, 1.0) > 0);
	BOOST_CHECK(tribool_compare(10.0, 10.0) == 0);

	BOOST_CHECK(tribool_compare(1, 2, std::greater<int>()) > 0);
	BOOST_CHECK(tribool_compare(2, 1, std::greater<int>()) < 0);
	BOOST_CHECK(tribool_compare(10, 10, std::greater<int>()) == 0);

}

BOOST_AUTO_TEST_CASE(ctpred_test)
{
	using namespace std;

	{
		set<string, ext::ctpred::less<ext::aci_string>> s;
		s.insert("test");
		s.insert("Test");
		s.insert("TeST");
		s.insert("Val");
		s.insert("VaL");
		BOOST_CHECK(s.size() == 2);
	}
	
	{
		set<string, ext::ctpred::less<string>> s;
		s.insert("test");
		s.insert("Test");
		s.insert("TeST");
		s.insert("Val");
		s.insert("VaL");
		BOOST_CHECK(s.size() == 5);
	}
	
}