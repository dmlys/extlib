#pragma once
#include <ostream>
#include <functional>
#include <ext/strings/basic_string_facade.hpp>
#include <boost/functional/hash.hpp>

namespace ext
{
	/************************************************************************/
	/*                integration                                           */
	/************************************************************************/
	namespace detail
	{
		template <class stream_char_type, class stream_char_traits>
		static bool pad_ostream(std::basic_streambuf<stream_char_type, stream_char_traits> * buf,
		                        int fch, std::size_t pad)
		{
			for (; 0 < pad; --pad)
			{
				bool fail = stream_char_traits::eq(stream_char_traits::eof(), buf->sputc(fch));
				if (fail) return false;
			}

			return true;
		}
	} // namespace detail
	
	template <
	    class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	std::basic_ostream<stream_char_type, stream_char_traits> &
		operator <<(std::basic_ostream<stream_char_type, stream_char_traits> & os,
	                const basic_string_facade<storage, char_traits> & str)
	{
		typedef std::basic_ostream<stream_char_type, stream_char_traits> ostream_type;
		typedef ext::basic_string_facade<storage, char_traits> string_type;
		typedef typename string_type::size_type size_type;

		std::ios_base::iostate state = std::ios_base::goodbit;
		const typename ostream_type::sentry ok(os);

		if (!ok)
			state |= std::ios::badbit;
		else
		{
			auto * buf = os.rdbuf();
			std::streamsize width = os.width();
			auto adjust = os.flags() & std::ios::adjustfield;
			auto fch = os.fill();

			auto data = str.data();
			size_type size = str.size();
			size_type pad = width <= 0 || static_cast<size_type>(width) <= size
				? 0
				: static_cast<size_type>(width) - size;

			try {
				if (adjust != std::ios::left)
				{
					// pad on left
					if (!detail::pad_ostream(buf, fch, pad))
						state |= std::ios::badbit;
				}

				if (state == std::ios::goodbit)
				{
					auto towrite = static_cast<std::streamsize>(size);
					auto written = buf->sputn(data, towrite);
					if (written != towrite)
						state |= std::ios::badbit;
					else if (adjust == std::ios::left)
					{
						// pad on right
						if (!detail::pad_ostream(buf, fch, pad))
							state |= std::ios::badbit;
					}
				}

				os.width(0);
			}
			catch (...)
			{
				state |= std::ios::badbit;
			}
		}
		
		os.setstate(state);
		return os;
	}

	template <
		class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	std::basic_istream<stream_char_type, stream_char_traits> &
		operator >>(std::basic_istream<stream_char_type, stream_char_traits> & is,
		            basic_string_facade<storage, char_traits> & str)
	{
		typedef std::ctype<stream_char_type> ctype_type;
		typedef std::basic_istream<stream_char_type, stream_char_traits> istream_type;
		typedef ext::basic_string_facade<storage, char_traits> string_type;
		typedef typename string_type::size_type size_type;

		std::ios_base::iostate state = std::ios_base::goodbit;
		const typename istream_type::sentry ok(is);

		if (ok)
		{
			const auto & lc = is.getloc();
			auto * buf = is.rdbuf();

			const ctype_type & ctype_fac = std::use_facet<ctype_type>(lc);
			str.clear();

			std::streamsize w = is.width();
			size_type size =
				0 < w && static_cast<size_type>(w) < str.max_size()
					? w
					: str.max_size();

			try {
				auto meta = buf->sgetc();
				for (; 0 < size; --size, meta = buf->snextc())
				{
					if (stream_char_traits::eq_int_type(stream_char_traits::eof(), meta))
					{
						// end of file
						state |= std::ios_base::eofbit;
						break;
					}

					auto ch = stream_char_traits::to_char_type(meta);
					if (ctype_fac.is(ctype_type::space, ch))
					{
						break;
					}

					str.push_back(ch);
				} // for
			}
			catch (...)
			{
				is.setstate(std::ios::badbit);
			}
		} // if ok
		
		is.width(0);
		if (str.empty()) // read nothing
			state |= std::ios_base::failbit;
		is.setstate(state);
		return is;
	}

	template <
		class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	std::basic_istream<stream_char_type, stream_char_traits> &
		getline(std::basic_istream<stream_char_type, stream_char_traits> && is,
		        basic_string_facade<storage, char_traits> & str,
		        stream_char_type delim)
	{
		typedef std::basic_istream<stream_char_type, stream_char_traits> istream_type;
		typedef stream_char_traits traits;
		
		std::ios_base::iostate state = std::ios_base::goodbit;
		const typename istream_type::sentry ok(is, true);

		if (ok)
		{
			str.clear();
			auto * buf = is.rdbuf();
			auto metadelim = traits::to_int_type(delim);
			auto meta = buf->sgetc();

			try {
				for (; ; meta = buf->snextc())
				{
					if (traits::eq_int_type(traits::eof(), meta))
					{
						state |= std::ios_base::eofbit;
						break;
					}

					if (traits::eq_int_type(meta, metadelim))
					{
						buf->sbumpc();
						break;
					}

					if (str.size() >= str.max_size())
					{
						state |= std::ios_base::failbit;
						break;
					}

					str += traits::to_char_type(meta);
				}
			}
			catch (...)
			{
				state |= std::ios_base::badbit;
			}
		}

		is.setstate(state);
		return is;
	}

	template <
		class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	inline
	std::basic_istream<stream_char_type, stream_char_traits> &
		getline(std::basic_istream<stream_char_type, stream_char_traits> & is,
		        basic_string_facade<storage, char_traits> & str, stream_char_type delim)
	{
		return getline(std::move(is), str, delim);
	}

	template <
		class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	inline
	std::basic_istream<stream_char_type, stream_char_traits> &
		getline(std::basic_istream<stream_char_type, stream_char_traits> && is,
		        basic_string_facade<storage, char_traits> & str)
	{
		return getline(std::move(is), str, '\n');
	}

	template <
		class stream_char_type, class stream_char_traits,
		class storage, class char_traits
	>
	inline
	std::basic_istream<stream_char_type, stream_char_traits> &
		getline(std::basic_istream<stream_char_type, stream_char_traits> & is,
		        basic_string_facade<storage, char_traits> & str)
	{
		return getline(std::move(is), str);
	}



	template <class storage, class char_traits>
	std::size_t hash_value(const basic_string_facade<storage, char_traits> & str)
	{
		typedef typename basic_string_facade<storage, char_traits>::value_type value_type;
		const value_type * first, *last;
		std::tie(first, last) = str.range();
		
		return boost::hash_range(first, last);
	}

} // namespace ext

namespace std
{
	template <class storage, class char_traits>
	struct hash<ext::basic_string_facade<storage, char_traits>>
	{
		std::size_t operator()(const ext::basic_string_facade<storage, char_traits> & str) const
		{
			return hash_value(str);
		}
	};
}
