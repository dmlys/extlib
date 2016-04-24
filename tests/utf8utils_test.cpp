#include <ext/utf8utils.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/locale/encoding.hpp>
#include <limits>

/// данный файл должен быть закодирован в utf8 without bom

using namespace std;
using boost::locale::conv::utf_to_utf;

bool test_u8trunc(std::string orig, std::size_t trsz, std::string const & expected)
{
	bool test1 = ext::utf8_utils::trunc_copy(orig, trsz) == expected;

	ext::utf8_utils::trunc(orig, trsz);
	bool test2 = orig == expected;
	return test1 && test2;
}

bool test_u8trunc(std::string const & orig, std::size_t trsz, std::wstring const & expected)
{
	return test_u8trunc(orig, trsz, utf_to_utf<char>(expected));
}

BOOST_AUTO_TEST_CASE(utf8utils_seqlen_test)
{
	using ext::utf8_utils::is_seqbeg;
	for (char ch = 0; ch < 0x7F; ++ch)
		BOOST_CHECK(is_seqbeg(ch) == true);

	for (unsigned u = 0xC0; u < 256; ++u)
	{
		unsigned char uch = u;
		BOOST_CHECK(is_seqbeg(uch) == true);
	}
	
}

BOOST_AUTO_TEST_CASE(utf8utils_test)
{
	//std::string u8str = utf_to_utf<char>("кириллица ascii русс");
	std::string u8str = "кириллица ascii русс";
	BOOST_CHECK(test_u8trunc(u8str, 0, ""));
	BOOST_CHECK(test_u8trunc(u8str, 1, ""));
	
	// в тесте кириллические буквы занимают по 2 байта
	// "кир" занимает 6 байт
	BOOST_CHECK(test_u8trunc(u8str, 4, "ки"));
	BOOST_CHECK(test_u8trunc(u8str, 5, "ки"));
	BOOST_CHECK(test_u8trunc(u8str, 6, "кир"));
	BOOST_CHECK(test_u8trunc(u8str, 7, "кир"));
	// "кири" занимает 8 байт
	BOOST_CHECK(test_u8trunc(u8str, 8, "кири"));
	// "кирил" занимает 9 байт
	BOOST_CHECK(test_u8trunc(u8str, 9, "кири"));
	BOOST_CHECK(test_u8trunc(u8str, 10, "кирил"));
	
	// "кириллица" занимает 18 байт
	BOOST_CHECK(test_u8trunc(u8str, 17, "кириллиц"));
	BOOST_CHECK(test_u8trunc(u8str, 18, "кириллица"));
	// + пробел - 19
	BOOST_CHECK(test_u8trunc(u8str, 19, "кириллица "));
	// + ascii - 24
	BOOST_CHECK(test_u8trunc(u8str, 23, "кириллица asci"));
	BOOST_CHECK(test_u8trunc(u8str, 24, "кириллица ascii"));
	// + пробел + 4
	BOOST_CHECK(test_u8trunc(u8str, 28, "кириллица ascii р"));
	BOOST_CHECK(test_u8trunc(u8str, 29, "кириллица ascii ру"));
	
	BOOST_CHECK(test_u8trunc(u8str, 32, "кириллица ascii рус"));
	BOOST_CHECK(test_u8trunc(u8str, 33, "кириллица ascii русс"));

	BOOST_CHECK(test_u8trunc(u8str, 1000, "кириллица ascii русс"));
}