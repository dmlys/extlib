#include <string>
#include <map>
#include <ext/string_interpolation.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_string_interpolation)
{
	std::map<std::string, std::string> values = {
			{"var", "123"},
			{"$var$", "$123$"},
			{"with space", "space value"},
			{"", "empty_key"},
			{"{bracket key}", "bracket_key_val"}
	};

	std::string templ;
	std::string result;
	std::string expected_result;
	
	templ = "test $var";
	expected_result= "test 123";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "test $var, comma after";
	expected_result = "test 123, comma after";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "test $\\$var\\$, comma after";
	expected_result = "test $123$, comma after";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "test ${var}";
	expected_result= "test 123";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "test \\$var";
	expected_result = "test $var";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);




	templ = "test $var$var, comma after";
	expected_result = "test 123123, comma after";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);


	templ = "test $var${var}, comma after";
	expected_result = "test 123123, comma after";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);



	templ = "word ${with space}";
	expected_result= "word space value";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);
	
	templ = "word \\${with space} and ${with space}";
	expected_result= "word ${with space} and space value";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);



	templ = "word ${\\{bracket key\\}} and $var, trailing";
	expected_result= "word bracket_key_val and 123, trailing";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "empty ${} test";
	expected_result = "empty empty_key test";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

	templ = "test $var${\\{bracket key\\}}$var, comma after";
	expected_result = "test 123bracket_key_val123, comma after";
	result = ext::interpolate(templ, values);
	BOOST_CHECK_EQUAL(result, expected_result);

}

BOOST_AUTO_TEST_CASE(test_string_interpolation_extreme)
{
	std::map<std::string, std::string> values;

	std::string alone_dollar = "word $ end";
	std::string result;

	result = ext::interpolate(alone_dollar, values);
	BOOST_CHECK_EQUAL(result, alone_dollar);

	std::string end_dollar = "word $";
	result = ext::interpolate(end_dollar, values);
	BOOST_CHECK_EQUAL(result, end_dollar);

	std::string unclosed = "word ${123";
	std::string unclosed2 = "word ${";

	BOOST_CHECK_EQUAL(unclosed, ext::interpolate(unclosed, values));
	BOOST_CHECK_EQUAL(unclosed2, ext::interpolate(unclosed2, values));
	
}