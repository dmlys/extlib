#pragma once
#include <ext/range.hpp>
#include <boost/iostreams/write.hpp>

namespace ext {
namespace iostreams
{
	template <class Sink>
	inline void write_all(Sink & sink, const typename boost::iostreams::char_type_of<Sink>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(sink, s, n);
		do {
			auto res = boost::iostreams::write(sink, s, n);
			if (res < 0) throw std::ios::failure("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		} while (n);
	}

	template<typename T, typename Sink>
	inline void write_all(T & t, Sink & sink, const typename boost::iostreams::char_type_of<T>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(t, sink, s, n);
		do {
			auto res = boost::iostreams::write(t, sink, s, n);
			if (res < 0) throw std::ios::failure("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		} while (n);
	}

	template <class Sink, class String>
	inline void write_string(Sink & sink, const String & str)
	{
		auto str_lit = ext::as_literal(str);
		auto * ptr = ext::data(str_lit);
		auto len = static_cast<std::streamsize>(str_lit.size());
		
		write_all(sink, ptr, len);
	}
}}
