#pragma once
#include <type_traits>
#include <limits>

namespace ext
{
	namespace iota_detail
	{
		template <class Arithmetic, class CharType>
		CharType * unsigned_itoa(Arithmetic val, CharType * buffer_end, unsigned radix)
		{
			static_assert(std::is_unsigned<Arithmetic>::value, "");
			// 2 < radix < 36: 10 + 26
			*--buffer_end = 0; // null terminator

			do {
				*--buffer_end = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[val % radix];
				val /= radix;
			} while (val);

			return buffer_end;
		}

		template <class Arithmetic, class CharType>
		CharType * signed_itoa(Arithmetic val, CharType * buffer_end)
		{
			typedef typename std::make_unsigned<Arithmetic>::type UnsignedType;

			// radix == 10 always

			// 0u - val - to silence warning, at least on msvc
			//
			// we need to get mod(val) into unsigned
			// signed to unsigned conversion in case of i < 0 are done as if:
			// unsigned u = UTYPE_MAX + 1 - mod(i)
			// so we need underflow it like 0 - static_cast<UnsignedType>(val)
			UnsignedType uval = val > 0 ? static_cast<UnsignedType>(val) : 0u - static_cast<UnsignedType>(val);
			buffer_end = iota_detail::unsigned_itoa(uval, buffer_end, 10);

			if (val < 0) *--buffer_end = '-';
			return buffer_end;
		}
	}

	/// converts val to string into buffer with buffer_size
	/// it prints to right border of the buffer
	/// so buffer + buffer_size - 1 == 0, buffer + buffer_size - 2 == last digit, and so on
	/// returns pointer to the beginning of result
	/// radix can be [2..36), if not - radix = 10
	template <class Arithmetic, class CharType>
	inline CharType * unsafe_itoa(Arithmetic val, CharType * buffer, std::size_t buffer_size, unsigned radix)
	{
		static_assert(std::is_arithmetic<Arithmetic>::value, "not a arithmetic type");
		typedef typename std::make_unsigned<Arithmetic>::type unsgined_type;

		if (radix < 2 || 36 < radix)
			radix = 10;

		if (std::is_unsigned<Arithmetic>::value || radix != 10)
			return iota_detail::unsigned_itoa(static_cast<unsgined_type>(val), buffer + buffer_size, radix);
		else
			return iota_detail::signed_itoa(val, buffer + buffer_size);
	}

	/// same as unsafe_itoa(val, buffer, BufferSize, 10), but checks that buffer is long enough
	template <class Arithmetic, class CharType, std::size_t BufferSize>
	inline CharType * itoa(Arithmetic val, CharType (& buffer)[BufferSize])
	{
		static_assert(std::is_arithmetic<Arithmetic>::value, "not a arithmetic type");
		// 1 - for null terminator, 1 - for last not fully representable group, see digits10 description
		const int ExpectedBufferSize = 2 + std::numeric_limits<Arithmetic>::digits10
			+ (std::is_signed<Arithmetic>::value ? 1 : 0);
		static_assert(BufferSize >= ExpectedBufferSize, "buffer is to small");

		return unsafe_itoa(val, buffer, BufferSize, 10);
	}
}
