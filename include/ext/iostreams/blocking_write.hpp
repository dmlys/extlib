#pragma once
#include <boost/iostreams/write.hpp>

namespace ext {
namespace iostreams
{
	template <class Sink>
	inline
	void blocking_write(Sink & sink, const typename boost::iostreams::char_type_of<Sink>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(sink, s, n);
		while (n) {
			auto res = boost::iostreams::write(sink, s, n);
			if (res < 0) throw std::ios::failure("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		};
	}

	template<typename T, typename Sink>
	inline
	void blocking_write(T & t, Sink & sink, const typename boost::iostreams::char_type_of<T>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(t, sink, s, n);
		while (n) {
			auto res = boost::iostreams::write(t, sink, s, n);
			if (res < 0) throw std::ios::failure("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		};
	}

}}