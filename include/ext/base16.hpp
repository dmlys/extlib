#pragma once
#include <ciso646>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <ext/range.hpp>
#include <ext/type_traits.hpp>
#include <ext/iostreams/write.hpp>

namespace ext
{
	namespace base16
	{
		namespace encoding_tables
		{
			static_assert(CHAR_BIT == 8, "byte is not octet");

			/// table used for hex encoding: 0 - '0' ... 10 - 'A' ... 15 - 'F'
			extern const char hex_encoding_array[16];
			extern const char hex_decoding_array[256];
		}

		/// base exception for all base16/hex errors
		class base16_exception : public std::runtime_error
		{
		public:
			base16_exception(const char * msg) : std::runtime_error(msg) {}
		};

		/// input has not enough input
		class not_enough_input : public base16_exception
		{
		public:
			not_enough_input() : base16_exception("ext::base16::decode: not full base16/hex group") {}
		};

		/// input has non valid hex char
		class non_hex_char : public base16_exception
		{
		public:
			non_hex_char() : base16_exception("ext::base16::decode: bad hex char in base16/hex group") {}
		};

		inline constexpr std::size_t encode_estimation(std::size_t size)
		{
			return size * 2;
		}

		inline constexpr std::size_t decode_estimation(std::size_t size)
		{
			return size / 2;
		}


		/// encodes symbol ch, as '<hex1><hex2>'
		template <class OutputIterator>
		inline constexpr OutputIterator & encode_char(OutputIterator & out, unsigned char ch)
		{
			*out = encoding_tables::hex_encoding_array[ch / 16];   ++out;			
			*out = encoding_tables::hex_encoding_array[ch % 16];   ++out;
			return out;
		}

		inline constexpr char decode_nibble(char ch)
		{
			ch = encoding_tables::hex_decoding_array[static_cast<unsigned char>(ch)];
			if (ch >= 0) return ch;

			throw non_hex_char();
		}

		template <class Iterator>
		inline constexpr char decode_char(Iterator & it)
		{
			return decode_nibble(*it++) * 16 + decode_nibble(*it++);
		}
	}


	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator<OutputIterator>::value, OutputIterator>
	encode_base16(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base16;

		for (; first != last; ++first)
			encode_char(out, *first);

		return out;
	}

	/// encodes text from range into container out
	template <class InputRange, class OutputContainer>
	std::enable_if_t<ext::is_range<OutputContainer>::value>
	encode_base16(const InputRange & input, OutputContainer & out)
	{
		using namespace base16;
		auto inplit = ext::as_literal(input);

		// we are appending
		auto out_size = encode_estimation(boost::size(inplit));
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto first = boost::begin(inplit);
		auto last = boost::end(inplit);
		auto out_beg = boost::begin(out) + old_size;
		//auto out_end = boost::end(out);

		encode_base16(first, last, out_beg);
	}

	/// encodes text from range into container out
	template <class OutputContainer = std::string, class InputRange>
	OutputContainer encode_base16(const InputRange & input)
	{
		OutputContainer out;
		encode_base16(input, out);
		return out;
	}


	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<not ext::is_iterator<Sink>::value>
	encode_base16(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// encoding produces more than input
		using namespace base16;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = decode_estimation(buffer_size);
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = encode_base16(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<not ext::is_range<Sink>::value>
	encode_base16(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		encode_base16(boost::begin(inplit), boost::end(inplit), out);
	}



	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator<OutputIterator>::value, OutputIterator>
	decode_base16(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base16;
		if ((last - first) % 2) throw not_enough_input();

		for (; first != last; ++out)
			*out = decode_char(first);

		return out;
	}
	
	template <class InputRange, class OutputContainer>
	std::enable_if_t<ext::is_range<OutputContainer>::value>
	decode_base16(const InputRange & input, OutputContainer & out)
	{
		using namespace base16;
		auto inplit = ext::as_literal(input);
		
		// we are appending
		auto out_size = decode_estimation(boost::size(inplit));
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto first = boost::begin(inplit);
		auto last = boost::end(inplit);
		auto out_beg = boost::begin(out) + old_size;

		decode_base16(first, last, out_beg);
	}

	template <class OutputContainer = std::string, class InputRange>
	OutputContainer decode_base16(const InputRange & input)
	{
		OutputContainer out;
		decode_base16(input, out);
		return out;
	}

	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<not ext::is_iterator<Sink>::value>
	decode_base16(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// decoding never produces more than input
		using namespace base16;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = buffer_size;
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = decode_base16(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<not ext::is_range<Sink>::value>
	decode_base16(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		decode_base16(boost::begin(inplit), boost::end(inplit), out);
	}
}
