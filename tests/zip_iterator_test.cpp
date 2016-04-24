#include <ext/zip_iterator.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(zip_iterator_sort_test)
{
	using namespace std;
	vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	vector<short> short_data {data.begin(), data.end()};
	boost::reverse(data);

	auto first = ext::make_zip_iterator(data.begin(), short_data.begin());
	auto last = ext::make_zip_iterator(data.end(), short_data.end());

	typedef decltype(first) iter_type;
	typedef std::tuple<const int &, const short &> ref;

	std::stable_sort(first, last, [](const ref & r1, const ref & r2) { return std::get<0>(r1) < std::get<0>(r2); });
	bool reversed = boost::is_sorted(short_data, std::greater<> {});
	BOOST_CHECK(reversed);
}

BOOST_AUTO_TEST_CASE(zip_iterator_partition_test)
{
	using namespace std;
	vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	vector<short> short_data {data.begin(), data.end()};

	auto expected_data = data;
	auto expected_short_data = short_data;

	// partition by odd
	boost::stable_partition(expected_data, [](int i) { return i % 1; });
	boost::stable_partition(expected_short_data, [](short i) { return i % 1; });

	auto first = ext::make_zip_iterator(data.begin(), short_data.begin());
	auto last = ext::make_zip_iterator(data.end(), short_data.end());
	
	//partition by odd on first array
	typedef std::tuple<const int &, const short &> ref;
	std::stable_partition(first, last, [](const ref & r) { return std::get<0>(r) % 1; });

	BOOST_CHECK(expected_data == data);
	BOOST_CHECK(expected_short_data == short_data);
}