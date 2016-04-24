#include <string>
#include <sstream>

#include <ext/range/line_reader.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(line_reader_test)
{
	std::string input;
	input += "first string\n";

	std::string long_str = "long string which is longer than chunk read size";
	input += long_str;

	std::stringstream ss {input};
	size_t chunk_size = 4;

	auto lines = ext::read_lines(ss, '\n', chunk_size);
	auto size = std::distance(lines.begin(), lines.end());
	BOOST_CHECK_EQUAL(size, 2);
}
