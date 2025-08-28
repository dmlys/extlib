#if EXT_ENABLE_OPENSSL

#include <openssl/asn1.h>
#include <ext/openssl.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(openssl_asn1_time_test)
{
	ASN1_TIME testval;
	
	std::string_view x509_date_str = "30201231091936Z";
	
	testval.data = (unsigned char *)x509_date_str.data();
	testval.length = x509_date_str.length();
	testval.type = V_ASN1_GENERALIZEDTIME;
	testval.flags = ASN1_STRING_FLAG_MSTRING;
	
	auto result = ext::openssl::asn1_time_string(&testval, ext::openssl::localtime, "%F %X");
	BOOST_CHECK_EQUAL(result, "3020-12-31 09:19:36");
}

BOOST_AUTO_TEST_CASE(openssl_asn1_time_inverse_test)
{
	std::tm tm;
	std::memset(&tm, 0, sizeof tm);
	sscanf("2025-08-30", "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
	tm.tm_mon -= 1;
	tm.tm_year -= 1900;

	ASN1_TIME * time = ext::openssl::asn1_time_tm(nullptr, &tm);
	BOOST_CHECK_EQUAL(time->type, V_ASN1_UTCTIME);
	BOOST_CHECK_EQUAL(reinterpret_cast<char *>(time->data), "250830000000Z");
	
	
	sscanf("3025-08-30", "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
	tm.tm_mon -= 1;
	tm.tm_year -= 1900;
	
	time = ext::openssl::asn1_time_tm(time, &tm);
	BOOST_CHECK_EQUAL(time->type, V_ASN1_GENERALIZEDTIME);
	BOOST_CHECK_EQUAL(reinterpret_cast<char *>(time->data), "30250830000000Z");
	
	ASN1_TIME_free(time);
}

#endif
