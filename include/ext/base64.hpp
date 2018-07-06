#pragma once
#include <ciso646>
#include <string>
#include <algorithm>
#include <ext/range.hpp>
#include <ext/type_traits.hpp>
#include <ext/iostreams/utility.hpp>

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

		constexpr std::size_t encode_estimation(std::size_t size)
		{
			return (size + InputGroupSize - 1) / InputGroupSize * OutputGroupSize;
		}

		constexpr std::size_t decode_estimation(std::size_t size)
		{
			return (size + OutputGroupSize - 1) / OutputGroupSize * InputGroupSize;
		}
	}

	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator_v<OutputIterator>, OutputIterator>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		using namespace base64;

		typedef encode_itearator<RandomAccessIterator> base64_iterator;
		auto count = InputGroupSize - (last - first) % InputGroupSize;
		count %= InputGroupSize;
		
		out = std::copy(base64_iterator(first), base64_iterator(last), out);
		out = std::fill_n(out, count, '=');
		return out;
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class RandomAccessIterator, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputContainer & out)
	{
		typedef base64::encode_itearator<RandomAccessIterator> base64_itearator;

		auto out_size = base64::encode_estimation(last - first);
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto out_beg = boost::begin(out) + old_size;
		auto out_end = boost::end(out);

		auto stopped = std::copy(base64_itearator(first), base64_itearator(last), out_beg);
		std::fill(stopped, out_end, '=');
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class InputRange, class OutputContainer>
	inline std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	encode_base64(const InputRange & input, OutputContainer & out)
	{
		auto inplit = ext::as_literal(input);
		return encode_base64(boost::begin(inplit), boost::end(inplit), out);
	}

	/// encodes text from range into container out, the last group is padded with '='
	template <class OutputContainer = std::string, class InputRange>
	OutputContainer encode_base64(const InputRange & input)
	{
		OutputContainer out;
		ext::encode_base64(input, out);
		return out;
	}


	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	encode_base64(RandomAccessIterator first, RandomAccessIterator last, Sink & sink)
	{
		// encoding produces more than input
		using namespace base64;
		constexpr std::size_t buffer_size = 256;
		constexpr std::size_t step_size = base64::decode_estimation(buffer_size);
		char buffer[buffer_size];

		while (first < last)
		{
			auto step_last = first + std::min<std::ptrdiff_t>(step_size, last - first);
			auto buf_end = ext::encode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}

		return sink;
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	encode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		return ext::encode_base64(boost::begin(inplit), boost::end(inplit), out);
	}



	template <class RandomAccessIterator, class OutputIterator>
	std::enable_if_t<ext::is_iterator_v<OutputIterator>, OutputIterator>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputIterator out)
	{
		typedef base64::decode_itearator<RandomAccessIterator> base64_iterator;
		return std::copy(base64_iterator(first), base64_iterator(last), out);
	}
	
	template <class RandomAccessIterator, class OutputContainer>
	std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	decode_base64(RandomAccessIterator first, RandomAccessIterator last, OutputContainer & out)
	{
		typedef base64::decode_itearator<RandomAccessIterator> base64_itearator;

		// we are appending
		auto out_size = base64::decode_estimation(last - first);
		auto old_size = out.size();
		out.resize(old_size + out_size);

		auto out_beg = boost::begin(out) + old_size;
		std::copy(base64_itearator(first), base64_itearator(last), out_beg);
	}

	template <class InputRange, class OutputContainer>
	inline std::enable_if_t<ext::is_container_v<OutputContainer>/*, OutputContainer &*/>
	decode_base64(const InputRange & input, OutputContainer & out)
	{
		auto inplit = ext::as_literal(input);
		return decode_base64(boost::begin(inplit), boost::end(inplit), out);
	}

	template <class OutputContainer = std::string, class InputRange>
	OutputContainer decode_base64(const InputRange & input)
	{
		OutputContainer out;
		ext::decode_base64(input, out);
		return out;
	}

	template <class RandomAccessIterator, class Sink>
	std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
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
			auto buf_end = ext::decode_base64(first, step_last, buffer);
			ext::iostreams::write_all(sink, buffer, buf_end - buffer);
			first = step_last;
		}

		return sink;
	}

	template <class InputRange, class Sink>
	inline std::enable_if_t<ext::iostreams::is_device_v<Sink>, Sink &>
	decode_base64(const InputRange & input, Sink & out)
	{
		auto inplit = ext::as_literal(input);
		return ext::decode_base64(boost::begin(inplit), boost::end(inplit), out);
	}
}
