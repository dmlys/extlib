#pragma once
#include <memory>
#include <istream>
#include <string>
#include <algorithm>
#include <ext/range/input_range_facade.hpp>

namespace ext
{
	/// range that lazy reads istream by lines
	/// it does the same thing as std::getline, but:
	///   * it provides range interface
	///   * it's faster, because it can read by chunks, not by one char
	///     (it will store extra read characters in internal buffer)
	///     that also mean that current get pos in istream is undefined
	///
	/// @Param CharType istream char_type, char/wchar_t/char16_t/char32_t
	/// @Param StringType, string_type range uses to return lines, std::string, boost::container::string, etc
	template <class CharType, class StringType = std::basic_string<CharType>>
	class basic_line_reader :
		public ext::input_range_facade<basic_line_reader<CharType, StringType>, StringType>
	{
	public:
		typedef CharType char_type;
		typedef StringType string_type;
		typedef std::basic_istream<char_type> istream_type;

		static const std::size_t def_chunk_size = 4096;
		static const char_type def_new_line = '\n';

	private:
		string_type line; // current line
		bool exhausted = false;

		std::size_t chunkSize; // read chunk size
		std::unique_ptr<char_type[]> buffer;
		char_type * bufend = nullptr;
		char_type * lastReaded = nullptr; // position in buffer where we stopped last time

		char_type newLine;  // newLine character
		istream_type * is;

		/// you can specialize this function for your particular string_type,
		/// if it does not support append method
		void append(string_type & str, const char_type * beg, const char_type * end)
		{
			str.append(beg, end);
		}

	public:
		string_type & front() { return line; }
		bool empty() const { return exhausted && line.empty(); }

		void pop_front()
		{
			line.clear();
			// while stream is valid or buffer isn't exhausted
			while (!exhausted)
			{
				if (lastReaded == bufend)
				{
					//peek and readsome for std::cin and a like streams
					//it will process as much as possible before blocking occurs
					bufend = lastReaded = buffer.get();
					if (is->peek() != istream_type::traits_type::eof())
						bufend += is->readsome(lastReaded, chunkSize);
					exhausted = lastReaded == bufend;
				}

				auto beg = lastReaded;
				lastReaded = std::find(lastReaded, bufend, newLine);
				append(line, beg, lastReaded);
				if (lastReaded != bufend)
				{
					++lastReaded;
					break;
				}
			}
		}

		basic_line_reader(istream_type & is, char_type newLine = def_new_line, std::size_t chunkSize = def_chunk_size)
			: chunkSize(chunkSize), newLine(newLine), is(&is)
		{
			buffer = std::make_unique<char_type[]>(chunkSize);
			pop_front();
		}

		//basic_line_reader(const basic_line_reader &) = delete;
		//basic_line_reader & operator =(const basic_line_reader &) = delete;
	};

	typedef basic_line_reader<char> line_reader;
	typedef basic_line_reader<wchar_t> wline_reader;

	template <class CharType>
	basic_line_reader<CharType> read_lines(std::basic_istream<CharType> & is,
	                                       CharType newLine = basic_line_reader<CharType>::def_new_line,
	                                       std::size_t chunkSize = basic_line_reader<CharType>::def_chunk_size)
	{
		return basic_line_reader<CharType>(is, newLine, chunkSize);
	}
}