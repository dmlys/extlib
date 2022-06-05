#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <new>
#include <limits>
#include <utility> // for std::pair
#include <stdexcept>

#include <boost/predef.h>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer/static_log2.hpp>
#include <ext/config.hpp>

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4200 4201)
#endif

namespace ext
{
	namespace compact_string_detail
	{
		enum
		{
			INPLACE,
			HEAP_SHORT,
			HEAP_LONG,
			HEAP_HUGE,
		};

		// typebit is garaunteed by std::max_align_t, see static assert bellow
		constexpr int TYPE_BIT = 2;
		BOOST_STATIC_ASSERT(boost::static_log2<sizeof(std::max_align_t)>::value >= TYPE_BIT);

		struct heap_short_body
		{
			static constexpr int type = HEAP_SHORT;

			std::uint8_t capacity;
			std::uint8_t size;
			char buffer[];
		};

		struct heap_long_body
		{
			static constexpr int type = HEAP_LONG;

			std::uint16_t capacity;
			std::uint16_t size;
			char buffer[];
		};

		struct heap_huge_body
		{
			static constexpr int type = HEAP_HUGE;

			std::size_t capacity;
			std::size_t size;
			char buffer[];
		};


		constexpr std::size_t HEAPSHORT_CAPACITY = (std::numeric_limits<std::uint8_t>::max)() - sizeof(heap_short_body);
		constexpr std::size_t HEAPLONG_CAPACITY  = (std::numeric_limits<std::uint16_t>::max)() - sizeof(heap_long_body);
		constexpr std::size_t HEAPHUGE_CAPACITY  = (std::numeric_limits<std::size_t>::max)();

		template <class Ariphmetic>
		inline void set_val(Ariphmetic & dest, std::size_t newval)
		{
			dest = static_cast<Ariphmetic>(newval);
		}


		template <class body_type>
		inline body_type * alloc_body_nothrow(std::size_t capacity)
		{
			return static_cast<body_type *>(::malloc(sizeof(body_type) + capacity));
		}

		inline void free_body(void * ptr)
		{
			::free(ptr);
		}

		template <class body_type>
		inline body_type * alloc_body(std::size_t capacity)
		{
			auto * body = alloc_body_nothrow<body_type>(capacity);
			if (body == nullptr) throw std::bad_alloc();
			return body;
		}

		template <class body_type>
		inline body_type * alloc_body_nothrow(std::size_t capacity, const char * first, std::size_t len)
		{
			auto * body = alloc_body_nothrow<body_type>(capacity);
			if (body == nullptr) return nullptr;

			set_val(body->capacity, capacity);
			set_val(body->size, len);
			std::memcpy(body->buffer, first, len);
			return body;
		}

		template <class body_type>
		inline body_type * alloc_body(std::size_t capacity, const char * first, std::size_t len)
		{
			auto * body = alloc_body<body_type>(capacity);
			set_val(body->capacity, capacity);
			set_val(body->size, len);
			std::memcpy(body->buffer, first, len);
			return body;
		}

		template <class body_type>
		inline body_type * alloc_copy(const body_type * other)
		{
			return alloc_body<body_type>(other->capacity, other->buffer, other->size);
		}


		template <class body_type, class other_body_type>
		inline typename std::enable_if< ! std::is_same<body_type, other_body_type>::value, body_type>::type *
			realloc_body_nothrow(other_body_type * other, std::size_t newcap)
		{
			std::size_t size = std::min<std::size_t>(other->size, newcap);
			auto * ptr = ::realloc(other, sizeof(body_type) + newcap);
			if (ptr == nullptr) return nullptr;
			
			auto * body = static_cast<body_type *>(ptr);
			other = static_cast<other_body_type *>(ptr);

			std::memmove(body->buffer, other->buffer, size);
			set_val(body->size, size);
			set_val(body->capacity, newcap);
			return body;
		}

		template <class body_type, class other_body_type>
		inline typename std::enable_if<std::is_same<body_type, other_body_type>::value, body_type>::type *
			realloc_body_nothrow(other_body_type * other, std::size_t newcap)
		{
			auto * body = static_cast<body_type *>(::realloc(other, sizeof(body_type) + newcap));
			if (body == nullptr) return nullptr;

			set_val(body->capacity, newcap);
			if (body->size > newcap) set_val(body->size, newcap);
			return body;
		}

		template <class body_type, class other_body_type>
		inline typename std::enable_if< ! std::is_same<body_type, other_body_type>::value, body_type>::type *
			realloc_body(other_body_type * other, std::size_t newcap)
		{
			std::size_t size = std::min<std::size_t>(other->size, newcap);
			auto * ptr = ::realloc(other, sizeof(body_type) + newcap);
			if (ptr == nullptr) throw std::bad_alloc();
			
			auto * body = static_cast<body_type *>(ptr);
			other = static_cast<other_body_type *>(ptr);

			std::memmove(body->buffer, other->buffer, size);
			set_val(body->size, size);
			set_val(body->capacity, newcap);
			return body;
		}

		template <class body_type, class other_body_type>
		inline typename std::enable_if<std::is_same<body_type, other_body_type>::value, body_type>::type *
			realloc_body(other_body_type * other, std::size_t newcap)
		{
			auto * body = static_cast<body_type *>(::realloc(other, sizeof(body_type) + newcap));
			if (body == nullptr) throw std::bad_alloc();

			set_val(body->capacity, newcap);
			if (body->size > newcap) set_val(body->size, newcap);
			return body;
		}
		

		template <unsigned inplace_size>
		struct compact_string_body
		{
			BOOST_STATIC_ASSERT(inplace_size >= sizeof(uintptr_t));
			BOOST_STATIC_ASSERT(inplace_size <= 1 << (CHAR_BIT - TYPE_BIT));

			union
			{
				struct // external
				{
					std::uint8_t type : TYPE_BIT;
					std::uint8_t      : 0;
					std::uintptr_t extptr;
				};

				struct // inplace
				{
					std::uint8_t        : TYPE_BIT;
					std::uint8_t inplen : CHAR_BIT - TYPE_BIT;
					char inpbuf[inplace_size - 1];
				};
			};
			

			static constexpr std::size_t INPLACE_CAPACITY   = inplace_size - 1;
			static constexpr std::size_t HEAPSHORT_CAPACITY = compact_string_detail::HEAPSHORT_CAPACITY;
			static constexpr std::size_t HEAPLONG_CAPACITY  = compact_string_detail::HEAPLONG_CAPACITY;
			static constexpr std::size_t HEAPHUGE_CAPACITY  = compact_string_detail::HEAPHUGE_CAPACITY;


			template <class body>
			inline static body * unpack(std::uintptr_t extptr) noexcept
			{
				return reinterpret_cast<body *>(extptr);
			}

			inline static std::uintptr_t pack(const void * extptr) noexcept
			{
				return reinterpret_cast<std::uintptr_t>(extptr);
			}
		};

		// sizeof pointer specialization
		template <>
		struct compact_string_body<sizeof(uintptr_t)>
		{
			union
			{
				struct // external
				{
					std::uintptr_t type   : TYPE_BIT;
					std::uintptr_t extptr : sizeof(std::uintptr_t) * CHAR_BIT - TYPE_BIT;
				};

				struct // inplace
				{
					std::uint8_t        : TYPE_BIT;
					std::uint8_t inplen : CHAR_BIT - TYPE_BIT;
					char inpbuf[sizeof(std::uintptr_t) - 1];
				};
			};

			static constexpr std::size_t INPLACE_CAPACITY   = sizeof(std::uintptr_t) - 1;
			static constexpr std::size_t HEAPSHORT_CAPACITY = compact_string_detail::HEAPSHORT_CAPACITY;
			static constexpr std::size_t HEAPLONG_CAPACITY  = compact_string_detail::HEAPLONG_CAPACITY;
			static constexpr std::size_t HEAPHUGE_CAPACITY  = compact_string_detail::HEAPHUGE_CAPACITY;


			template <class body>
			inline static body * unpack(std::uintptr_t extptr) noexcept
			{
				return reinterpret_cast<body *>(extptr << TYPE_BIT);
			}

			inline static std::uintptr_t pack(const void * extptr) noexcept
			{
				return reinterpret_cast<std::uintptr_t>(extptr) >> TYPE_BIT;
			}
		};
	} // namespace compact_string_detail

	template <unsigned InplaceSize>
	class compact_string_base : private compact_string_detail::compact_string_body<InplaceSize>
	{
		typedef compact_string_base                                       self_type;
		typedef compact_string_detail::compact_string_body<InplaceSize>   base_type;

	public:
		static constexpr auto inplace_size = InplaceSize;

	public:
		typedef                   char          value_type;
		typedef            std::size_t          size_type;
		typedef         std::ptrdiff_t          difference_type;

		typedef std::pair<value_type *, value_type *>              range_type;
		typedef std::pair<const value_type *, const value_type *>  const_range_type;

	private:
		typedef compact_string_detail::heap_short_body heap_short_body;
		typedef compact_string_detail::heap_long_body heap_long_body;
		typedef compact_string_detail::heap_huge_body  heap_huge_body;

		using base_type::INPLACE_CAPACITY;
		using base_type::HEAPSHORT_CAPACITY;
		using base_type::HEAPLONG_CAPACITY;
		using base_type::HEAPHUGE_CAPACITY;

		using base_type::type;
		using base_type::extptr;
		using base_type::inpbuf;
		using base_type::inplen;

		using base_type::pack;
		using base_type::unpack;

	private:
		template <class SizeType>
		static SizeType decrease_size(SizeType cursize, std::size_t decsize);
		static std::size_t increase_size(std::size_t cursize, std::size_t incsize);

		static std::size_t grow_capacity(std::size_t newcap, std::size_t oldcap);
		static std::size_t shrink_capacity(std::size_t newcap, std::size_t oldcap);

		template <class body_type>
		void set_body(body_type * body);

		value_type * resize_inplace(std::size_t newsize, bool change_size);
		value_type * resize_body_adjusted(heap_short_body * oldbody, std::size_t newsize, bool change_size);
		value_type * resize_body_adjusted(heap_long_body * oldbody, std::size_t newsize, bool change_size);
		value_type * resize_body_adjusted(heap_huge_body  * oldbody, std::size_t newsize, bool change_size);

		value_type * shrink_body(heap_short_body * oldbody, std::size_t newsize);
		value_type * shrink_body(heap_long_body * oldbody, std::size_t newsize);
		value_type * shrink_body(heap_huge_body * oldbody, std::size_t newsize);

	public:
		      value_type * data()       noexcept;
		const value_type * data() const noexcept;

		      value_type * data_end()       noexcept;
		const value_type * data_end() const noexcept;

		      range_type range()       noexcept;
		const_range_type range() const noexcept;

		static size_type max_size() noexcept { return (std::numeric_limits<size_type>::max)(); }
		size_type capacity() const  noexcept;
		size_type size()     const  noexcept;
		bool empty()         const  noexcept { return size() == 0; }

	public:
		BOOST_NOINLINE void resize(size_type newsize);
		BOOST_NOINLINE void reserve(size_type newcap);
		BOOST_NOINLINE void shrink_to_fit();

	public:
		BOOST_NORETURN static void throw_xlen() { throw std::length_error("length_error"); }

	protected:
		BOOST_FORCEINLINE static void set_eos(value_type *) {}
		BOOST_NOINLINE value_type * grow_to(size_type newsize);
		BOOST_NOINLINE std::pair<value_type *, size_type> grow_by(size_type size_increment);
		BOOST_NOINLINE std::pair<value_type *, size_type> shrink_by(size_type size_decrement);

	public:
		compact_string_base() noexcept { static_cast<base_type &>(*this) = {}; };
		~compact_string_base() noexcept;

		compact_string_base(const compact_string_base & copy);
		compact_string_base(compact_string_base && copy) noexcept;
		compact_string_base & operator =(const compact_string_base & copy);
		compact_string_base & operator =(compact_string_base && copy) noexcept;

		template <unsigned inplace_size>
		friend void swap(compact_string_base<inplace_size> & s1, compact_string_base<inplace_size> & s2) noexcept;
	};

	template <unsigned InplaceSize>
	template <class SizeType>
	inline SizeType ext::compact_string_base<InplaceSize>::decrease_size(SizeType cursize, std::size_t decsize)
	{
		assert(decsize <= cursize);
		return static_cast<SizeType>(cursize - decsize);
	}

	template <unsigned InplaceSize>
	inline std::size_t ext::compact_string_base<InplaceSize>::increase_size(std::size_t cursize, std::size_t incsize)
	{
		incsize = cursize + incsize;
		// overflow or more than max_size
		// size_type is unsigned so overflow is well defined
		if (incsize < cursize || incsize >= (std::numeric_limits<std::size_t>::max)())
			throw_xlen();

		return incsize;
	}

	template <unsigned InplaceSize>
	inline std::size_t ext::compact_string_base<InplaceSize>::grow_capacity(std::size_t newcap, std::size_t oldcap)
	{
		constexpr auto dpointer = 2 * sizeof(std::uintptr_t);
		constexpr auto qpointer = 4 * sizeof(std::uintptr_t);

		if (newcap <= dpointer) return dpointer;
		if (newcap <= qpointer) return qpointer;

		auto cap = (oldcap / 2 <= newcap / 3) ? newcap : oldcap + oldcap / 2;
		if (cap < newcap) cap = newcap;

		return cap;
	}


	template <unsigned InplaceSize>
	inline std::size_t ext::compact_string_base<InplaceSize>::shrink_capacity(std::size_t newcap, std::size_t oldcap)
	{
		constexpr auto dpointer = 2 * sizeof(std::uintptr_t);
		constexpr auto qpointer = 4 * sizeof(std::uintptr_t);

		if (newcap <= dpointer) return dpointer;
		if (newcap <= qpointer) return qpointer;

		return newcap;
	}

	/************************************************************************/
	/*                 inline methods                                       */
	/************************************************************************/
	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::data() noexcept -> value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:     return inpbuf;
			case HEAP_SHORT:  return base_type::template unpack<heap_short_body>(extptr)->buffer;
			case HEAP_LONG:   return base_type::template unpack<heap_long_body>(extptr)->buffer;
			case HEAP_HUGE:   return base_type::template unpack<heap_huge_body>(extptr)->buffer;

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::data() const noexcept -> const value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:     return inpbuf;
			case HEAP_SHORT:  return base_type::template unpack<heap_short_body>(extptr)->buffer;
			case HEAP_LONG:   return base_type::template unpack<heap_long_body>(extptr)->buffer;
			case HEAP_HUGE:   return base_type::template unpack<heap_huge_body>(extptr)->buffer;

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::data_end() noexcept -> value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return inpbuf + inplen;

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				return body->buffer + body->size;
			}
			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				return body->buffer + body->size;
			}
			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				return body->buffer + body->size;
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::data_end() const noexcept -> const value_type *
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return inpbuf + inplen;

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				return body->buffer + body->size;
			}
			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				return body->buffer + body->size;
			}
			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				return body->buffer + body->size;
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::range() noexcept -> range_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return {inpbuf, inpbuf + inplen};

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}
			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::range() const noexcept -> const_range_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:
				return {inpbuf, inpbuf + inplen};

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}
			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				return {
					body->buffer,
					body->buffer + body->size
				};
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::size() const noexcept -> size_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:      return inplen;
			case HEAP_SHORT:   return base_type::template unpack<heap_short_body>(extptr)->size;
			case HEAP_LONG:    return base_type::template unpack<heap_long_body>(extptr)->size;
			case HEAP_HUGE:    return base_type::template unpack<heap_huge_body>(extptr)->size;

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline auto compact_string_base<InplaceSize>::capacity() const noexcept->size_type
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:      return inplen;
			case HEAP_SHORT:   return base_type::template unpack<heap_short_body>(extptr)->size;
			case HEAP_LONG:    return base_type::template unpack<heap_long_body>(extptr)->size;
			case HEAP_HUGE:    return base_type::template unpack<heap_huge_body>(extptr)->size;

			default: EXT_UNREACHABLE();
		}
	}

	/************************************************************************/
	/*                  ctors/dtors                                         */
	/************************************************************************/
	template <unsigned InplaceSize>
	compact_string_base<InplaceSize>::~compact_string_base() noexcept
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return;

			case HEAP_SHORT:
			case HEAP_LONG:
			case HEAP_HUGE:
				free_body(base_type::template unpack<void>(base_type::extptr));
				return;

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	inline compact_string_base<InplaceSize>::compact_string_base(compact_string_base && other) noexcept
	{
		using namespace compact_string_detail;
		base_type::operator =(other);
		static_cast<base_type &>(other) = {};
	}

	template <unsigned InplaceSize>
	inline compact_string_base<InplaceSize> & compact_string_base<InplaceSize>::operator =(compact_string_base && other) noexcept
	{
		this->~compact_string_base();
		base_type::operator =(other);
		static_cast<base_type &>(other) = {};
		return *this;
	}

	template <unsigned InplaceSize>
	inline void swap(compact_string_base<InplaceSize> & s1, compact_string_base<InplaceSize> & s2) noexcept
	{
		using compact_string_detail::compact_string_body;
		std::swap(static_cast<compact_string_body<InplaceSize> &>(s1), static_cast<compact_string_body<InplaceSize> &>(s1));
	}

	template <unsigned InplaceSize>
	compact_string_base<InplaceSize>::compact_string_base(const compact_string_base & other)
	{
		using namespace compact_string_detail;

		type = other.type;
		switch (type)
		{
			case INPLACE:
				base_type::operator =(other);
				return;

			case HEAP_SHORT:
				extptr = pack(alloc_copy(base_type::template unpack<heap_short_body>(other.extptr)));
				return;

			case HEAP_LONG:
				extptr = pack(alloc_copy(base_type::template unpack<heap_long_body>(other.extptr)));
				return;

			case HEAP_HUGE:
				extptr = pack(alloc_copy(base_type::template unpack<heap_huge_body>(other.extptr)));
				return;

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	compact_string_base<InplaceSize> & compact_string_base<InplaceSize>::operator =(const compact_string_base & other)
	{
		if (this != &other)
		{
			this->~compact_string_base();
			new (this) compact_string_base(other);
		}

		return *this;
	}

	/************************************************************************/
	/*                  internal body manipulation methods                  */
	/************************************************************************/
	template <unsigned InplaceSize>
	template <class body_type>
	inline void compact_string_base<InplaceSize>::set_body(body_type * body)
	{
		extptr = pack(body);
		type = body->type;
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::resize_inplace(size_type newsize, bool change_size) -> value_type *
	{
		using namespace compact_string_detail;
		if (newsize <= INPLACE_CAPACITY)
		{
			if (change_size) inplen = static_cast<decltype(inplen)>(newsize);
			return inpbuf;
		}

		auto first = inpbuf;
		auto size = inplen;

		auto cap = grow_capacity(newsize, size);

		heap_short_body * (* short_alloc)(std::size_t, const char *, std::size_t) = &alloc_body_nothrow<heap_short_body>;
		heap_long_body  * (* long_alloc)(std::size_t, const char *, std::size_t)  = &alloc_body_nothrow<heap_long_body>;
		heap_huge_body  * (* huge_alloc)(std::size_t, const char *, std::size_t)  = &alloc_body_nothrow<heap_huge_body>;

	again:
		if (newsize <= HEAPSHORT_CAPACITY)
		{
			auto * newbody = short_alloc(cap, first, size);
			if (newbody == nullptr) goto alloc_fail;

			if (change_size) set_val(newbody->size, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
		else if (newsize <= HEAPLONG_CAPACITY)
		{
			auto * newbody = long_alloc(cap, first, size);
			if (newbody == nullptr) goto alloc_fail;

			if (change_size) set_val(newbody->size, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
		else
		{
			auto * newbody = huge_alloc(cap, first, size);
			if (newbody == nullptr) goto alloc_fail;

			if (change_size) set_val(newbody->size, newsize);
			set_body(newbody);
			return newbody->buffer;
		}

	alloc_fail:
		cap = newsize;

		short_alloc = &alloc_body<heap_short_body>;
		long_alloc  = &alloc_body<heap_long_body>;
		huge_alloc  = &alloc_body<heap_huge_body>;

		goto again;
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::resize_body_adjusted(heap_short_body * oldbody, std::size_t newsize, bool change_size)
		-> value_type *
	{
		using namespace compact_string_detail;

		if (newsize <= oldbody->capacity)
		{
			if (change_size) set_val(oldbody->size, newsize);
			return oldbody->buffer;
		}

		auto cap = grow_capacity(newsize, oldbody->capacity);

		auto * short_realloc = &realloc_body_nothrow<heap_short_body, heap_short_body>;
		auto * long_realloc =  &realloc_body_nothrow<heap_long_body, heap_short_body>;
		auto * huge_realloc =  &realloc_body_nothrow<heap_huge_body, heap_short_body>;

	again:
		if (cap <= HEAPSHORT_CAPACITY)
		{
			heap_short_body * body = short_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}
		else if (cap <= HEAPLONG_CAPACITY)
		{
			heap_long_body * body = long_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}
		else
		{
			heap_huge_body * body = huge_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}


	alloc_fail:
		cap = newsize;

		short_realloc = &realloc_body<heap_short_body, heap_short_body>;
		long_realloc =  &realloc_body<heap_long_body, heap_short_body>;
		huge_realloc =  &realloc_body<heap_huge_body, heap_short_body>;

		goto again;
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::resize_body_adjusted(heap_long_body * oldbody, std::size_t newsize, bool change_size)
		-> value_type *
	{
		using namespace compact_string_detail;

		if (newsize <= oldbody->capacity)
		{
			if (change_size) set_val(oldbody->size, newsize);
			return oldbody->buffer;
		}

		auto cap = grow_capacity(newsize, oldbody->capacity);

		auto * long_realloc = &realloc_body_nothrow<heap_long_body, heap_long_body>;
		auto * huge_realloc = &realloc_body_nothrow<heap_huge_body, heap_long_body>;

	again:
		if (cap <= HEAPLONG_CAPACITY)
		{
			heap_long_body * body = long_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}
		else
		{
			heap_huge_body * body = huge_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}


	alloc_fail:
		cap = newsize;

		long_realloc = &realloc_body<heap_long_body, heap_long_body>;
		huge_realloc = &realloc_body<heap_huge_body, heap_long_body>;

		goto again;
	}
	
	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::resize_body_adjusted(heap_huge_body * oldbody, std::size_t newsize, bool change_size)
		-> value_type *
	{
		using namespace compact_string_detail;

		if (newsize <= oldbody->capacity)
		{
			if (change_size) set_val(oldbody->size, newsize);
			return oldbody->buffer;
		}
		
		auto cap = grow_capacity(newsize, oldbody->capacity);
		auto * huge_realloc = &realloc_body_nothrow<heap_huge_body, heap_huge_body>;

	again:
		{
			heap_huge_body * body = huge_realloc(oldbody, cap);
			if (body == nullptr) goto alloc_fail;

			if (change_size) set_val(body->size, newsize);
			set_body(body);
			return body->buffer;
		}


	alloc_fail:
		cap = newsize;
		huge_realloc = &realloc_body<heap_huge_body, heap_huge_body>;
		goto again;
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::shrink_body(heap_short_body * oldbody, std::size_t newsize)
		-> value_type *
	{
		using namespace compact_string_detail;

		std::size_t oldcap = oldbody->capacity;
		if (newsize == oldcap) return oldbody->buffer;
		assert(newsize < oldcap);

		if (newsize <= INPLACE_CAPACITY)
		{
			std::memcpy(inpbuf, oldbody->buffer, newsize);
			inplen = static_cast<decltype(inplen)>(newsize);
			type = INPLACE;
			free_body(oldbody);
			return inpbuf;
		}
		else
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_short_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::shrink_body(heap_long_body * oldbody, std::size_t newsize)
		-> value_type *
	{
		using namespace compact_string_detail;

		std::size_t oldcap = oldbody->capacity;
		if (newsize == oldcap) return oldbody->buffer;
		assert(newsize < oldcap);

		if (newsize <= INPLACE_CAPACITY)
		{
			std::memcpy(inpbuf, oldbody->buffer, newsize);
			inplen = static_cast<decltype(inplen)>(newsize);
			type = INPLACE;
			free_body(oldbody);
			return inpbuf;
		}
		else if (newsize <= HEAPSHORT_CAPACITY)
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_short_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
		else
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_long_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
	}

	template <unsigned InplaceSize>
	auto compact_string_base<InplaceSize>::shrink_body(heap_huge_body * oldbody, std::size_t newsize)
		-> value_type *
	{
		using namespace compact_string_detail;

		std::size_t oldcap = oldbody->capacity;
		if (newsize == oldcap) return oldbody->buffer;
		assert(newsize < oldcap);

		if (newsize <= INPLACE_CAPACITY)
		{
			std::memcpy(inpbuf, oldbody->buffer, newsize);
			inplen = static_cast<decltype(inplen)>(newsize);
			type = INPLACE;
			free_body(oldbody);
			return inpbuf;
		}
		else if (newsize <= HEAPSHORT_CAPACITY)
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_short_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
		else if (newsize <= HEAPLONG_CAPACITY)
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_long_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
		else
		{
			newsize = shrink_capacity(newsize, oldcap);
			auto * newbody = realloc_body<heap_huge_body>(oldbody, newsize);
			set_body(newbody);
			return newbody->buffer;
		}
	}


	/************************************************************************/
	/*                    capacity changing methods                         */
	/************************************************************************/
	template <unsigned InplaceSize>
	BOOST_NOINLINE auto compact_string_base<InplaceSize>::grow_to(size_type newsize) -> value_type *
	{
		using namespace compact_string_detail;
		if (newsize >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
				return resize_inplace(newsize, true);

			case HEAP_SHORT:
			{
				auto * oldbody = base_type::template unpack<heap_short_body>(extptr);
				return resize_body_adjusted(oldbody, newsize, true);
			}

			case HEAP_LONG:
			{
				auto * oldbody = base_type::template unpack<heap_long_body>(extptr);
				return resize_body_adjusted(oldbody, newsize, true);
			}

			case HEAP_HUGE:
			{
				auto * oldbody = base_type::template unpack<heap_huge_body>(extptr);
				return resize_body_adjusted(oldbody, newsize, true);
			}

			default: EXT_UNREACHABLE();
		}
	}
	
	template <unsigned InplaceSize>
	BOOST_NOINLINE auto compact_string_base<InplaceSize>::grow_by(size_type size_increment)
		-> std::pair<value_type *, size_type>
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE:
			{
				auto newsize = increase_size(inplen, size_increment);
				auto * retptr = resize_inplace(newsize, true);
				return {retptr, newsize};
			}

			case HEAP_SHORT:
			{
				auto * oldbody = base_type::template unpack<heap_short_body>(extptr);
				auto newsize = increase_size(oldbody->size, size_increment);
				auto * retptr = resize_body_adjusted(oldbody, newsize, true);
				return {retptr, newsize};
			}

			case HEAP_LONG:
			{
				auto * oldbody = base_type::template unpack<heap_long_body>(extptr);
				auto newsize = increase_size(oldbody->size, size_increment);
				auto * retptr = resize_body_adjusted(oldbody, newsize, true);
				return {retptr, newsize};
			}

			case HEAP_HUGE:
			{
				auto * oldbody = base_type::template unpack<heap_huge_body>(extptr);
				auto newsize = increase_size(oldbody->size, size_increment);
				auto * retptr = resize_body_adjusted(oldbody, newsize, true);
				return {retptr, newsize};
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	BOOST_NOINLINE auto compact_string_base<InplaceSize>::shrink_by(size_type size_decrement)
		-> std::pair<value_type *, size_type>
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE:
			{
				inplen = decrease_size(inplen, size_decrement);
				return {static_cast<value_type *>(inpbuf), static_cast<size_type>(inplen)};
			}

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				body->size = decrease_size(body->size, size_decrement);
				return {body->buffer, body->size};
			}

			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				body->size = decrease_size(body->size, size_decrement);
				return {body->buffer, body->size};
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				body->size = decrease_size(body->size, size_decrement);
				return {body->buffer, body->size};
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	BOOST_NOINLINE void compact_string_base<InplaceSize>::resize(size_type newsize)
	{
		using namespace compact_string_detail;
		if (newsize >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
				resize_inplace(newsize, true);
				return;

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				resize_body_adjusted(body, newsize, true);
				return;
			}

			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				resize_body_adjusted(body, newsize, true);
				return;
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				resize_body_adjusted(body, newsize, true);
				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	BOOST_NOINLINE void compact_string_base<InplaceSize>::reserve(size_type newcap)
	{
		using namespace compact_string_detail;
		if (newcap >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
				resize_inplace(newcap, false);
				return;

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				resize_body_adjusted(body, newcap, false);
				return;
			}

			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				resize_body_adjusted(body, newcap, false);
				return;
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				resize_body_adjusted(body, newcap, false);
				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

	template <unsigned InplaceSize>
	BOOST_NOINLINE void compact_string_base<InplaceSize>::shrink_to_fit()
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE: return;

			case HEAP_SHORT:
			{
				auto * body = base_type::template unpack<heap_short_body>(extptr);
				shrink_body(body, body->size);
				return;
			}

			case HEAP_LONG:
			{
				auto * body = base_type::template unpack<heap_long_body>(extptr);
				shrink_body(body, body->size);
				return;
			}

			case HEAP_HUGE:
			{
				auto * body = base_type::template unpack<heap_huge_body>(extptr);
				shrink_body(body, body->size);
				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

} // namespace ext

#if _MSC_VER
#pragma warning(pop)
#endif
