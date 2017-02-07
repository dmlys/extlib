#pragma once
#include <ciso646>
#include <algorithm>
#include <ext/is_iterator.hpp>
#include <ext/range.hpp>
#include <ext/iostreams/write.hpp>

#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>


namespace ext
{
	namespace base64
	{
		constexpr unsigned OutputGroupSize = 4;
		constexpr unsigned InputGroupSize = 3;

		template <class Iterator>
		using encode_itearator = 
			boost::archive::iterators::base64_from_binary<
				boost::archive::iterators::transform_width<Iterator, 6, 8>
			>;

		template <class Iterator>
		using decode_itearator = 
			boost::archive::iterators::transform_width<
				boost::archive::iterators::binary_from_base64<Iterator>, 8, 6
			>;

		inline constexpr std::size_t encode_estimation(std::size_t size)
		{
			return (size + InputGroupSize - 1) / InputGroupSize * OutputGroupSize;
		}

		inline constexpr std::size_t decode_estimation(std::size_t size)
		{
			return (size + OutputGroupSize - 1) / OutputGroupSize * InputGroupSize;
		}
	}

	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator<OutputIterator>::value, OutputIterator>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base64;

		typedef encode_itearator<RandomAccessIterator> base64_iterator;
		auto count = InputGroupSize - (last - first) % InputGroupSize;
		
		out = std::copy(base64_iterator(first), base64_iterator(last), out);
		out = std::fill_n(out, count, '=');
		return out;
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class InputRange, class OutputContainer>
	std::enable_if_t<ext::is_range<OutputContainer>::value>
	encode_base64(const InputRange & input, OutputContainer & out)
	{
		using namespace base64;
		auto inplit = ext::as_literal(input);

		typedef typename boost::range_iterator<decltype(inplit)>::type input_iterator;
		typedef encode_itearator<input_iterator> base64_itearator;

		auto out_size = encode_estimation(boost::size(inplit));
		out.resize(out_size);		

		base64_itearator first = boost::begin(inplit);
		base64_itearator last = boost::end(inplit);
		auto out_beg = boost::begin(out);
		auto out_end = boost::end(out);

		auto stopped = std::copy(first, last, out_beg);
		std::fill(stopped, out_end, '=');
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class OutputContainer = std::string, class InputRange>
	OutputContainer encode_base64(const InputRange & input)
	{
		OutputContainer out;
		encode_base64(input, out);
		return out;
	}


	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<not ext::is_iterator<Sink>::value>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// encoding produces more than input
		using namespace base64;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = decode_estimation(buffer_size);
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = encode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<not ext::is_range<Sink>::value>
	encode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		encode_base64(boost::begin(inplit), boost::end(inplit), out);
	}



	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator<OutputIterator>::value, OutputIterator>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base64;

		typedef decode_itearator<RandomAccessIterator> base64_iterator;
		return std::copy(base64_iterator(first), base64_iterator(last), out);
	}
	
	template <class InputRange, class OutputContainer>
	std::enable_if_t<ext::is_range<OutputContainer>::value>
	decode_base64(const InputRange & input, OutputContainer & out)
	{
		using namespace base64;
		auto inplit = ext::as_literal(input);

		typedef typename boost::range_iterator<decltype(inplit)>::type input_iterator;
		typedef decode_itearator<input_iterator> base64_itearator;

		auto out_size = decode_estimation(boost::size(inplit));
		out.resize(out_size);

		base64_itearator first = boost::begin(inplit);
		base64_itearator last = boost::end(inplit);
		auto out_beg = boost::begin(out);
		auto out_end = boost::end(out);

		std::copy(first, last, out_beg);		
	}

	template <class OutputContainer = std::string, class InputRange>
	OutputContainer decode_base64(const InputRange & input)
	{
		OutputContainer out;
		decode_base64(input, out);
		return out;
	}

	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<not ext::is_iterator<Sink>::value>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// decoding never produces more than input
		using namespace base64;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = buffer_size;
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = decode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<not ext::is_range<Sink>::value>
	decode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		decode_base64(boost::begin(inplit), boost::end(inplit), out);
	}
}
