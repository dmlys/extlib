#include <vector>
#include <sstream>
#include <ext/iostreams/transform_width_encoder.hpp>
#include <ext/iostreams/transform_width_decoder.hpp>

#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/test.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <boost/test/unit_test.hpp>
#include "test_files.h"

namespace
{
	struct test_case
	{
		std::string normal;
		std::string single_encoded;
		std::string double_encoded;
		std::string triple_encoded;
	};

	typedef ext::iostreams::iterator_encoder<
		boost::archive::iterators::base64_from_binary<
			boost::archive::iterators::transform_width<const char *, 6, 8>
		>
	> base64_encoder_functor;

	typedef ext::iostreams::iterator_decoder<
		boost::archive::iterators::transform_width<
			boost::archive::iterators::binary_from_base64<const char *>,
			8, 6
		>
	> base64_decoder_functor;

	typedef ext::iostreams::transform_width_encoder<
		base64_encoder_functor, 0,
		3, 4, ext::iostreams::DefaultTransformWidthEncoderBufferSize
	> base64_encoder;

	typedef ext::iostreams::transform_width_decoder<
		base64_decoder_functor,
		4, 3, ext::iostreams::DefaultTransformWidthDecoderBufferSize, true
	> base64_decoder;

	typedef ext::iostreams::transform_width_decoder <
		base64_decoder_functor,
		4, 3, ext::iostreams::DefaultTransformWidthDecoderBufferSize, false
	> strict_base64_decoder;
	
	template <class Filter, class Range1, class Range2>
	inline bool test_output_filter(const Filter & filter, const Range1 & input, const Range2 & output)
	{
		return boost::iostreams::test_output_filter(filter, input, output);
	}

	template <class Filter, class Range1, class Range2>
	inline bool test_input_filter(const Filter & filter, const Range1 & input, const Range2 & output)
	{
		return boost::iostreams::test_input_filter(filter, input, output);
	}

	template <class Encoder>
	void test_encoder(Encoder encoder, const test_case & test)
	{
		auto double_encoder = boost::iostreams::compose(encoder, encoder);
		auto triple_encoder = boost::iostreams::compose(double_encoder, encoder);
		
		BOOST_REQUIRE(::test_output_filter(encoder, test.normal, test.single_encoded));
		BOOST_REQUIRE(::test_output_filter(encoder, test.single_encoded, test.double_encoded));
		BOOST_REQUIRE(::test_output_filter(encoder, test.double_encoded, test.triple_encoded));

		BOOST_REQUIRE(::test_output_filter(double_encoder, test.normal, test.double_encoded));
		BOOST_REQUIRE(::test_output_filter(double_encoder, test.single_encoded, test.triple_encoded));
		BOOST_REQUIRE(::test_output_filter(triple_encoder, test.normal, test.triple_encoded));
	}

	template <class Decoder>
	void test_decoder(Decoder decoder, const test_case & test)
	{
		auto double_decoder = boost::iostreams::compose(decoder, decoder);
		auto triple_decoder = boost::iostreams::compose(double_decoder, decoder);

		BOOST_REQUIRE(::test_input_filter(decoder, test.single_encoded, test.normal));
		BOOST_REQUIRE(::test_input_filter(decoder, test.double_encoded, test.single_encoded));
		BOOST_REQUIRE(::test_input_filter(decoder, test.triple_encoded, test.double_encoded));

		// commented due a bug in boost::iostreams
		//BOOST_REQUIRE(::test_input_filter(double_decoder, test.double_encoded, test.normal));
		//BOOST_REQUIRE(::test_input_filter(double_decoder, test.triple_encoded, test.single_encoded));
		//BOOST_REQUIRE(::test_input_filter(triple_decoder, test.triple_encoded, test.normal));
	}

	template <class Encoder>
	void test_encdoer_in_filter_chain(Encoder encoder, const test_case & test, std::vector<int> bufSizes)
	{
		for (int bufSize : bufSizes)
		{
			std::string output;
			{
				boost::iostreams::filtering_ostream os;
				os.push(encoder, bufSize);
				os.push(boost::iostreams::back_inserter(output));

				os << test.normal;
			}
			BOOST_REQUIRE(output == test.single_encoded);

			output.clear();
			{
				boost::iostreams::filtering_ostream os;
				os.push(encoder, bufSize);
				os.push(encoder, bufSize);
				os.push(encoder, bufSize);
				os.push(boost::iostreams::back_inserter(output));

				os << test.normal;
			}
			BOOST_REQUIRE(output == test.triple_encoded);
		}
	}

	template <class Decoder>
	void test_decdoer_in_filter_chain(Decoder decoder, const test_case & test, std::vector<int> bufSizes)
	{
		for (int bufSize : bufSizes)
		{
			std::string output;
			{
				boost::iostreams::filtering_istream is;
				is.push(decoder, bufSize);
				is.push(boost::make_iterator_range(test.single_encoded));

				std::stringstream ss;
				ss << is.rdbuf();
				output = ss.str();
			}
			BOOST_REQUIRE(output == test.normal);

			output.clear();
			{
				boost::iostreams::filtering_istream is;
				is.push(decoder, bufSize);
				is.push(decoder, bufSize);
				is.push(decoder, bufSize);
				is.push(boost::make_iterator_range(test.triple_encoded));

				std::stringstream ss;
				ss << is.rdbuf();
				output = ss.str();
			}
			BOOST_REQUIRE(output == test.normal);
		}
	}

}

BOOST_AUTO_TEST_CASE(transform_width_test)
{
	test_case tc;
	LoadTestFile("Base64TestFiles/DataLink32.ps1", tc.normal, std::ios::binary);
	LoadTestFile("Base64TestFiles/DataLink32.ps1.single_encoded", tc.single_encoded, std::ios::binary);
	LoadTestFile("Base64TestFiles/DataLink32.ps1.double_encoded", tc.double_encoded, std::ios::binary);
	LoadTestFile("Base64TestFiles/DataLink32.ps1.triple_encoded", tc.triple_encoded, std::ios::binary);

	test_encoder(base64_encoder(), tc);
	test_decoder(base64_decoder(), tc);
	test_decoder(strict_base64_decoder(), tc);


	auto bufSizes = {-1, 1, 128, 1234, 4096};
	test_encdoer_in_filter_chain(base64_encoder(), tc, bufSizes);
	test_decdoer_in_filter_chain(base64_decoder(), tc, bufSizes);
	test_decdoer_in_filter_chain(strict_base64_decoder(), tc, bufSizes);
}
