#include <string>
#include <map>
#include <ext/lrucache.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(manual_lru_cache_test)
{
	ext::manual_lru_cache<int, std::string> isc {5};
	std::string * io;
	io = isc.find_ptr(10);
	BOOST_CHECK(io == nullptr);

	isc.insert(10, "901245678");
	io = isc.find_ptr(10);
	BOOST_CHECK(*io == "901245678");

	isc.insert(11, "11");
	isc.insert(12, "12");
	isc.insert(13, "13");
	isc.insert(14, "14");
	// after haere will be drops

	BOOST_CHECK(isc.find_ptr(11) != nullptr);

	isc.insert(15, "15");
	isc.insert(16, "16");

	// it's touched at line 29, so it's must not be dropped
	BOOST_CHECK(isc.find_ptr(11) != nullptr);
	BOOST_CHECK(isc.find_ptr(12) == nullptr);
	BOOST_CHECK(isc.find_ptr(10) == nullptr);
}

BOOST_AUTO_TEST_CASE(lru_cache_test)
{
	std::map<int, unsigned> counters;
	auto source = [&counters] (int k)
	{
		++counters[k];
		return std::to_string(k);
	};

	ext::lru_cache<int, std::string> cache {5, source};

	cache.at(10);
	cache.at(11);
	cache.at(12);
	cache.at(13);
	cache.at(14);
	// cache filled

	// should not change counters
	cache.at(12);
	cache.at(14);
	cache.at(10);
	cache.at(11);
	cache.at(13);

	for (auto & val : counters) {
		BOOST_CHECK(val.second == 1);
	}

	// 12, 14 should be dropped
	cache.at(15);
	cache.at(16);

	BOOST_CHECK(counters[15] == 1);
	BOOST_CHECK(counters[16] == 1);

	// should reacquire
	cache.at(12);
	cache.at(14);

	BOOST_CHECK(counters[12] == 2);
	BOOST_CHECK(counters[14] == 2);
}
