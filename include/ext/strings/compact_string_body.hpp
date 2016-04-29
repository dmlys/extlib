#pragma once
#include <cstddef>
#include <cstdint>

#include <new>
#include <limits>
#include <utility> // for std::pair

#include <boost/predef.h>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer/static_log2.hpp>
#include <ext/config.hpp>

namespace ext
{

#define EXT_COMPACT_STRING_BODY_INLINE BOOST_FORCEINLINE
//#define EXT_COMPACT_STRING_BODY_INLINE inline

	namespace compact_string_detail
	{
		struct heap_normal_body
		{
			std::size_t capacity;
			std::size_t length;
			char buffer[1];
		};

		struct heap_compact_body
		{
			std::uint8_t length;
			char buffer[1];
		};

		// typebit is garaunteed by std::max_align_t, see static assert bellow
		static const int TYPE_BIT = 3;

		static const int INPLACE_CAPACITY = sizeof(std::uintptr_t) - 1;
		static const int HEAP16_CAPACITY = 16 - 1;
		static const int HEAP32_CAPACITY = 32 - 1;

		enum
		{
			INPLACE,
			HEAP16,
			HEAP32,
			HEAP,
		};

		EXT_COMPACT_STRING_BODY_INLINE int capacity_to_type(std::size_t capacity)
		{
			if      (capacity <= INPLACE_CAPACITY)    return INPLACE;
			else if (capacity <= HEAP16_CAPACITY)     return HEAP16;
			else if (capacity <= HEAP32_CAPACITY)     return HEAP32;
			else                                      return HEAP;
		}

		struct compact_string_body
		{
			union
			{
				struct // external
				{
					std::uintptr_t type : TYPE_BIT;
					std::uintptr_t extptr : sizeof(std::uintptr_t) * CHAR_BIT - TYPE_BIT;
				};

				struct // inplace
				{
					std::uint8_t        : TYPE_BIT;
					std::uint8_t inplen : CHAR_BIT - TYPE_BIT;
					char inpbuf[sizeof(std::uintptr_t) - 1];
				};
			};
		};

		template <class body>
		EXT_COMPACT_STRING_BODY_INLINE body * unpack(std::uintptr_t extptr)
		{
			return reinterpret_cast<body *>(extptr << TYPE_BIT);
		}

		EXT_COMPACT_STRING_BODY_INLINE std::uintptr_t pack(const void * extptr)
		{
			return reinterpret_cast<std::uintptr_t>(extptr) >> TYPE_BIT;
		}

		BOOST_STATIC_ASSERT(sizeof(std::uintptr_t) == sizeof(compact_string_body));
		BOOST_STATIC_ASSERT(boost::static_log2<sizeof(std::max_align_t)>::value >= 3);
	} // namespace compact_string_detail

	class compact_string_base : private compact_string_detail::compact_string_body
	{
		typedef compact_string_body base_type;
		typedef compact_string_base self_type;

	public:
		typedef                   char          value_type;
		typedef            std::size_t          size_type;
		typedef         std::ptrdiff_t          difference_type;

		typedef std::pair<value_type *, value_type *>              range_type;
		typedef std::pair<const value_type *, const value_type *>  const_range_type;

	public:
		      value_type * data()       BOOST_NOEXCEPT;
		const value_type * data() const BOOST_NOEXCEPT;

		      value_type * data_end()       BOOST_NOEXCEPT;
		const value_type * data_end() const BOOST_NOEXCEPT;

		      range_type range()       BOOST_NOEXCEPT;
		const_range_type range() const BOOST_NOEXCEPT;

		static size_type max_size() BOOST_NOEXCEPT { return (std::numeric_limits<size_type>::max)(); }
		size_type capacity() const  BOOST_NOEXCEPT;
		size_type size()     const  BOOST_NOEXCEPT;
		bool empty()         const  BOOST_NOEXCEPT { return size() == 0; }

	public:
		BOOST_NOINLINE void resize(size_type newsize);
		BOOST_NOINLINE void reserve(size_type newcap);
		BOOST_NOINLINE void shrink_to_fit();

	public:
		BOOST_NORETURN static void throw_xlen();

	protected:
		BOOST_FORCEINLINE static void set_eos(value_type *) {}
		BOOST_NOINLINE value_type * grow_to(size_type newsize);
		BOOST_NOINLINE std::pair<value_type *, size_type> grow_by(size_type size_increment);
		BOOST_NOINLINE std::pair<value_type *, size_type> shrink_by(size_type size_decrement);

	public:
		compact_string_base() BOOST_NOEXCEPT;
		~compact_string_base() BOOST_NOEXCEPT;

		compact_string_base(const compact_string_base & copy);
		compact_string_base(compact_string_base && copy) BOOST_NOEXCEPT;
		compact_string_base & operator =(const compact_string_base & copy);
		compact_string_base & operator =(compact_string_base && copy) BOOST_NOEXCEPT;

		friend void swap(compact_string_base & s1, compact_string_base & s2) BOOST_NOEXCEPT;
	};

	/************************************************************************/
	/*                 inline methods                                       */
	/************************************************************************/
	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::data() BOOST_NOEXCEPT -> value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return inpbuf;
			case HEAP16:
			case HEAP32:    return unpack<heap_compact_body>(extptr)->buffer;
			case HEAP:      return unpack<heap_normal_body>(extptr)->buffer;

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::data() const BOOST_NOEXCEPT -> const value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return inpbuf;
			case HEAP16:
			case HEAP32:    return unpack<heap_compact_body>(extptr)->buffer;
			case HEAP:      return unpack<heap_normal_body>(extptr)->buffer;

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::data_end() BOOST_NOEXCEPT -> value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return inpbuf + inplen;
			case HEAP16:
			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				return body->buffer + body->length;
			}
			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				return body->buffer + body->length;
			}

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::data_end() const BOOST_NOEXCEPT -> const value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return inpbuf + inplen;
			case HEAP16:
			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				return body->buffer + body->length;
			}
			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				return body->buffer + body->length;
			}

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::range() BOOST_NOEXCEPT -> range_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return {inpbuf, inpbuf + inplen};
			case HEAP16:
			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->length
				};
			}

			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->length
				};
			}

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::range() const BOOST_NOEXCEPT -> const_range_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return {inpbuf, inpbuf + inplen};
			case HEAP16:
			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->length
				};
			}

			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->length
				};
			}

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::size() const BOOST_NOEXCEPT -> size_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return inplen;
			case HEAP16:
			case HEAP32:    return unpack<heap_compact_body>(extptr)->length;
			case HEAP:      return unpack<heap_normal_body>(extptr)->length;

			default: EXT_UNREACHABLE();
		}
	}

	EXT_COMPACT_STRING_BODY_INLINE auto compact_string_base::capacity() const BOOST_NOEXCEPT->size_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return INPLACE_CAPACITY;
			case HEAP16:    return HEAP16_CAPACITY;
			case HEAP32:    return HEAP32_CAPACITY;
			case HEAP:      return unpack<heap_normal_body>(extptr)->capacity;

			default: EXT_UNREACHABLE();
		}
	}

	/************************************************************************/
	/*                  ctors/dtors                                         */
	/************************************************************************/
	inline compact_string_base::compact_string_base() BOOST_NOEXCEPT
		: base_type()
	{

	}

	inline compact_string_base::compact_string_base(compact_string_base && other) BOOST_NOEXCEPT
	{
		using namespace compact_string_detail;
		base_type::operator =(other);
		static_cast<base_type &>(other) = {};
	}

	inline compact_string_base & compact_string_base::operator =(compact_string_base && other) BOOST_NOEXCEPT
	{
		this->~compact_string_base();
		base_type::operator =(other);
		static_cast<base_type &>(other) = {};
		return *this;
	}

	inline void swap(compact_string_base & s1, compact_string_base & s2) BOOST_NOEXCEPT
	{
		using compact_string_detail::compact_string_body;
		std::swap(static_cast<compact_string_body &>(s1), static_cast<compact_string_body &>(s1));
	}

#undef EXT_COMPACT_STRING_BODY_INLINE
} // namespace ext

