#pragma once
#include <cstddef>
#include <string>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <ext/iostreams/utility.hpp>

namespace ext {
namespace iostreams
{
	/// boost::iostreams::filter. Splits input stream into lines delimeted by custom "linebreak" no longer than given size.
	/// This filter does not check input stream for '\r', '\n', and so does make any processing for them.
	/// It's main purpose to work in pair with filters like base64_encode_filter, qprintable_endcode_filter, etc.
	class linebreak_insert_filter :
		public boost::iostreams::multichar_output_filter
	{
	private:
		std::string m_lineBreak;
		std::size_t m_lineSize;
		std::size_t m_lastLineCount = 0;

	private:
		template <class Sink>
		static void do_sinkwrite(Sink & sink, const char * data, std::streamsize count)
		{
			write_all(sink, data, count);
		}

		template <class Sink>
		inline void write_crlf(Sink & sink)
		{
			do_sinkwrite(sink, m_lineBreak.data(), m_lineBreak.size());
		}

	public:
		template <class Sink>
		std::streamsize write(Sink & sink, const char * data, std::streamsize n)
		{
			std::size_t count = boost::numeric_cast<std::size_t>(n);
			while (count)
			{
				if (m_lastLineCount == m_lineSize) {
					write_crlf(sink);
					m_lastLineCount = 0;
				}

				std::size_t towrite = std::min(m_lineSize - m_lastLineCount, count);
				m_lastLineCount += towrite;
				count -= towrite;

				do_sinkwrite(sink, data, towrite);
				data += towrite;
			}

			return n;
		}

	public:
		std::size_t line_size() const { return m_lineSize; }
		const std::string & linebreak() const { return m_lineBreak; }

		linebreak_insert_filter(std::size_t lineSize, const std::string & lineBreak = "\r\n")
			: m_lineBreak(lineBreak), m_lineSize(lineSize) {}
	};

	BOOST_IOSTREAMS_PIPABLE(linebreak_insert_filter, 0);
}}