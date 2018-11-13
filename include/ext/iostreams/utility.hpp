#pragma once
#include <ext/type_traits.hpp>
#include <ext/range.hpp>
#include <boost/iostreams/write.hpp>

namespace ext::iostreams
{
	namespace iostreams_detail
	{
		template <class Type, class = ext::void_t<>>
		struct has_category : std::false_type {};

		template <class Type>
		struct has_category<Type, ext::void_t<typename Type::category>>
			: std::true_type {};

		template <class Type, class = ext::void_t<>>
		struct has_char_type : std::false_type {};

		template <class Type>
		struct has_char_type<Type, ext::void_t<typename Type::char_type>>
			: std::true_type {};
	}

	template <class Type>
	struct is_boost_iostreams_device :
		std::conjunction<
			iostreams_detail::has_category<Type>,
			iostreams_detail::has_char_type<Type>
		> 
	{};

	// either std::*stream or boost::iostreams::device
	template <class Type>
	struct is_device : 
		std::disjunction<
			std::is_base_of<std::ios_base, Type>,
			is_boost_iostreams_device<Type>
		> 
	{};		

	template <class Type>
	constexpr auto is_device_v = is_device<Type>::value;




	template <class Sink>
	void write_all(Sink & sink, const typename boost::iostreams::char_type_of<Sink>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(sink, s, n);
		do {
			auto res = boost::iostreams::write(sink, s, n);
			if (res < 0) throw std::runtime_error("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		} while (n);
	}

	template<typename T, typename Sink>
	void write_all(T & t, Sink & sink, const typename boost::iostreams::char_type_of<T>::type * s, std::streamsize n)
	{
		//boost::iostreams::write(t, sink, s, n);
		do {
			auto res = boost::iostreams::write(t, sink, s, n);
			if (res < 0) throw std::runtime_error("failed to write to underline sink");

			s += static_cast<std::size_t>(res);
			n -= res;
		} while (n);
	}

	template <class Sink, class String>
	inline void write_string(Sink & sink, const String & str)
	{
		auto str_lit = ext::str_view(str);
		auto * ptr = ext::data(str_lit);
		auto len = static_cast<std::streamsize>(str_lit.size());
		
		write_all(sink, ptr, len);
	}
}
