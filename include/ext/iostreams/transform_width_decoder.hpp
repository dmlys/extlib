#pragma once
#include <cstddef>
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <boost/iostreams/operations.hpp>

namespace ext {
namespace iostreams
{
	const std::size_t DefaultTransformWidthDecoderBufferSize = 4096;

	template <class TransformWidthIterator>
	struct iterator_decoder
	{
		template <class Iterator, class OutputIterator>
		inline OutputIterator operator()(Iterator first, Iterator last, OutputIterator out) const
		{
			return std::copy(TransformWidthIterator(first), TransformWidthIterator(last), out);
		}
	};

	/// boost::iostreams::input_filter. Decodes input binary stream into binary stream via DecodeFunctor
	/// This class manages stream processing and it's corner cases, like request size is not multiple of OutputGroupSize
	/// 
	/// DecodeFunctor must:
	///   * do conversion X * InputGroupSize bytes into Y * OutputGroupSize bytes
	///   * DecodeFunctor only needs to Decode given buffer into destination buffer pointed by iterator,
	///     destination buffer will always be large enough for passed input buffer
	///   * most time filter will call functor with input data length divisible by InputGroupSize,
	///     but if input data has trailing group at the end - filter will call functor with it,
	///     so it must be able to handle that data
	///   * next expressions must be valid:
	///     DecodeFunctor func;
	///       out = func(first, last, out);
	/// 
	/// @Param DecodeFunctor stateless DecodeFunctor
	/// @Param InputGroupSize input not decoded group size, must be > 0
	/// @Param OutputGroupSize output decoded group size, must be > 0
	/// @Param BufferSize internal buffer size used for decoding, must be >= InputGroupSize
	/// @Param ShortReads allows short reads.
	///          If true it can return less than asked:
	///             if asked buffer is not multiple of OutputGroupSize, it will read only full groups
	///             if underline source returned less than asked - filter will process only what read
	///             filter will always read something, unless underline source is exhausted
	///          if false it will always return exactly asked number of characters, until underline source is exhausted
	template <
		class DecodeFunctor,
		unsigned InputGroupSize,
		unsigned OutputGroupSize,
		std::size_t BufferSize = DefaultTransformWidthDecoderBufferSize,
		bool ShortReads = true
	>
	class transform_width_decoder
	{
	public:
		static_assert(InputGroupSize,  "InputGroupSize must be greater than 0");
		static_assert(OutputGroupSize, "OutputGroupSize must be greater than 0");
		static_assert(BufferSize >= InputGroupSize, "BufferSize must be >= InputGroupSize");

		typedef char char_type;

		struct category :
			boost::iostreams::multichar_input_filter_tag,
			boost::iostreams::optimally_buffered_tag
		{ };

		typedef DecodeFunctor decoder_type;

	private:
		static const std::size_t RoundedBufferSize = BufferSize / InputGroupSize * InputGroupSize;
		decoder_type m_decoder;

		// helper buffer for storing trailing groups
		char m_outStorebuf[OutputGroupSize];
		char m_inStorebuf[InputGroupSize];

		char * m_outStoreEnd = m_outStorebuf;
		char * m_outStorePos = m_outStoreEnd;
		unsigned m_inStorebufCount = 0;

	private:
		inline char * decode(const char * first, const char * last, char * out)
		{
			return m_decoder(first, last, out);
		}

		template <class Source>
		static std::size_t fill_buffer(Source & source, char * first, std::size_t tofill)
		{
			BOOST_ASSERT(tofill >= InputGroupSize);
			std::size_t minimumRead = ShortReads ? InputGroupSize : tofill;
			std::size_t read = 0;
			do {
				auto res = boost::iostreams::read(source, first, tofill - read);
				if (res < 0) break; //no more data

				read += static_cast<std::size_t>(res);
				first += static_cast<std::size_t>(res);
			} while (read < minimumRead);

			return read;
		}
		
		/// writes trailing data from last read operation
		/// adjusts data, size by written count
		void write_trailing_output(char * & data, std::size_t & size)
		{
			std::size_t consumed = m_outStoreEnd - m_outStorePos;
			if (consumed)
			{
				consumed = std::min(consumed, size);
				data = std::copy_n(m_outStorePos, consumed, data);
				size -= consumed;
				m_outStorePos += consumed;
			}
		}

		template <class Source>
		void process_trailing_output(Source & source, char * & data, std::size_t trailingCount)
		{
			BOOST_ASSERT(m_outStorePos == m_outStoreEnd);
			char buffer[InputGroupSize];
			auto read = fill_buffer(source, buffer, InputGroupSize);
			m_outStoreEnd = decode(buffer, buffer + read, m_outStorebuf);
			m_outStorePos = std::min(m_outStorebuf + trailingCount, m_outStoreEnd); // std::min handles empty read
			data = std::copy(m_outStorebuf, m_outStorePos, data);
		}

		void restore_trailing_input(char * & working_buffer)
		{
			working_buffer = std::copy_n(m_inStorebuf, m_inStorebufCount, working_buffer);
			m_inStorebufCount = 0;
		}

		void store_trailing_input(char * working_buffer, char * & working_buffer_end)
		{
			auto sz = working_buffer_end - working_buffer;
			auto trailing = sz % InputGroupSize;
			if (sz != trailing) // more than one group
			{
				working_buffer_end -= trailing;
				m_inStorebufCount = static_cast<unsigned>(trailing);
				std::copy_n(working_buffer_end, trailing, m_inStorebuf);
			}
		}

		inline std::streamsize process_return(std::streamsize processed)
		{
			return processed ? processed : EOF;
		}

	public:
		template <class Source>
		std::streamsize read(Source & source, char * data, std::streamsize n)
		{
			// on every call:
			//  * if we have something left from previous call - write it as much as possible
			//  * fill buffer from source, decode into target area,
			//    repeat until source is exhausted or area is filled rounded down by OutputGroupSize
			//    if read was partial(we read all source) - it's decoder responsibility to process last not full group
			//  
			//    short read support(if ShortReads is true):
			//      if read count was less then requested - process read full groups, and store not full group into internal buffer,
			//        but only if there is more than one group
			//      before first read restore saved data from internal into working buffer
			//
			//  * if target area not divisible by OutputGroupSize - decode into m_outStorebuf,
			//    copy into target area and store what's left for next call

			auto output = data;
			std::size_t count = boost::numeric_cast<std::size_t>(n);
			write_trailing_output(output, count);
			if (count == 0) return process_return(output - data);

			char buffer[RoundedBufferSize + InputGroupSize];
			
			bool read_more = true;
			std::size_t trailingOutput = count % OutputGroupSize;
			std::size_t roundedCount = count - trailingOutput;
			std::size_t read_count, toread;
			
			while (roundedCount && read_more)
			{
				auto curbuf = buffer;
				if (ShortReads) restore_trailing_input(curbuf);

				toread = roundedCount / OutputGroupSize * InputGroupSize;
				toread = std::min(toread, RoundedBufferSize);
				
				read_count = fill_buffer(source, curbuf, toread);
				curbuf += read_count;
				read_more = ShortReads ? read_count == toread : read_count > 0;
				if (ShortReads) store_trailing_input(buffer, curbuf);

				auto stopped = decode(buffer, curbuf, output);
				roundedCount -= stopped - output;
				output = stopped;
			}

			bool should_supply_trailing = ShortReads ? trailingOutput == count : read_more && trailingOutput;
			if (should_supply_trailing)
				process_trailing_output(source, output, trailingOutput);

			return process_return(output - data);
		}

		std::size_t optimal_buffer_size() const
		{
			return RoundedBufferSize / InputGroupSize * OutputGroupSize;
		}

	public:
		transform_width_decoder() = default;

		transform_width_decoder(const transform_width_decoder & op)
			: m_decoder(op.m_decoder)
		{
			std::copy_n(op.m_outStorebuf, OutputGroupSize, m_outStorebuf);
			std::copy_n(op.m_inStorebuf, OutputGroupSize, m_inStorebuf);
			m_inStorebufCount = op.m_inStorebufCount;
			m_outStoreEnd = m_outStorebuf + (op.m_outStoreEnd - op.m_outStorebuf);
			m_outStorePos = m_outStorebuf + (op.m_outStorePos - op.m_outStorebuf);
		}

		transform_width_decoder & operator = (const transform_width_decoder & op)
		{
			m_decoder = std::move(op.m_decoder);
			std::copy_n(op.m_outStorebuf, OutputGroupSize, m_outStorebuf);
			std::copy_n(op.m_inStorebuf, OutputGroupSize, m_inStorebuf);
			m_inStorebufCount = op.m_inStorebufCount;
			m_outStoreEnd = m_outStorebuf + (op.m_outStoreEnd - op.m_outStorebuf);
			m_outStorePos = m_outStorebuf + (op.m_outStorePos - op.m_outStorebuf);
			
			return *this;
		}
	};


	// definition of statics
	template <
		class DecodeFunctor,
		unsigned InputGroupSize,
		unsigned OutputGroupSize,
		std::size_t BufferSize,
		bool ShortReads
	>
	const std::size_t transform_width_decoder<
		DecodeFunctor,
		InputGroupSize,
		OutputGroupSize,
		BufferSize,
		ShortReads
	>::RoundedBufferSize;


	// boost::iostreams pipable intergration
	template <
		class DecodeFunctor,
		unsigned InputGroupSize,
		unsigned OutputGroupSize,
		std::size_t BufferSize,
		bool ShortReads,
		typename Component
	>
	::boost::iostreams::pipeline<
		::boost::iostreams::detail::pipeline_segment<
			transform_width_decoder<DecodeFunctor, InputGroupSize, OutputGroupSize, BufferSize, ShortReads>
		>,
		Component
	>
	operator |(const transform_width_decoder<DecodeFunctor, InputGroupSize, OutputGroupSize, BufferSize, ShortReads> & filter,
	           const Component & c)
	{
		typedef ::boost::iostreams::detail::pipeline_segment <
			transform_width_decoder<DecodeFunctor, InputGroupSize, OutputGroupSize, BufferSize, ShortReads>
		> segment;

		return ::boost::iostreams::pipeline<segment, Component>(segment(filter), c);
	}
}}

