#include <ext/strings/compact_string_body.hpp>
#include <cstring>
#include <stdexcept>

namespace ext
{
	namespace compact_string_detail
	{
		static inline heap_normal_body * alloc_normal_body(std::size_t capacity)
		{
			return reinterpret_cast<heap_normal_body *>(new char[sizeof(std::size_t) * 2 + capacity]);
		}

		static inline heap_normal_body * alloc_normal_body(std::nothrow_t, std::size_t capacity)
		{
			return reinterpret_cast<heap_normal_body *>(new(std::nothrow) char[sizeof(std::size_t) * 2 + capacity]);
		}

		static inline heap_compact_body * alloc_compact_body(unsigned capacity)
		{
			return reinterpret_cast<heap_compact_body *>(new char[sizeof(std::uint8_t) + capacity]);
		}

		static inline heap_normal_body * alloc_copy(const heap_normal_body * other)
		{
			auto * body = alloc_normal_body(other->capacity);
			std::memcpy(body, other, sizeof(std::size_t) * 2 + other->capacity);
			return body;
		}

		static inline heap_compact_body * alloc_copy(const heap_compact_body * other, unsigned capacity)
		{
			auto * body = alloc_compact_body(capacity);
			std::memcpy(body, other, sizeof(std::uint8_t) + capacity);
			return body;
		}

		static inline heap_compact_body * alloc_compact_body(unsigned capacity, const char * first, std::size_t len)
		{
			auto * body = alloc_compact_body(capacity);
			body->length = static_cast<std::uint8_t>(len);
			std::memcpy(body->buffer, first, len);
			return body;
		}

		static inline heap_normal_body * alloc_normal_body(std::size_t capacity, const char * first, std::size_t len)
		{
			auto * body = alloc_normal_body(capacity);
			body->capacity = capacity;
			body->length = static_cast<std::uint8_t>(len);
			std::memcpy(body->buffer, first, len);
			return body;
		}

		static inline heap_normal_body * alloc_normal_body_adjusted(std::size_t newcap, std::size_t oldcap,
		                                                            const char * first, std::size_t len)
		{
			heap_normal_body * body;
			auto cap = (oldcap / 2 <= newcap / 3) ? newcap : oldcap + oldcap / 2;
			
			// if was overflow or allocation failed
			if (cap < newcap || (body = alloc_normal_body(std::nothrow, cap)) == nullptr)
			{
				cap = newcap;
				body = alloc_normal_body(cap);
			}

			body->capacity = cap;
			body->length = len;
			std::memcpy(body->buffer, first, len);

			return body;
		}

	} // namespace compact_string_detail

	static compact_string_base::size_type increase_size(compact_string_base::size_type cursize,
	                                                    compact_string_base::size_type incsize)
	{
		incsize = cursize + incsize;
		// overflow or more than max_size
		// size_type is unsigned so overflow is well defined
		if (incsize < cursize || incsize >= compact_string_base::max_size())
			compact_string_base::throw_xlen();

		return incsize;
	}

	template <class SizeType>
	BOOST_FORCEINLINE
	static SizeType decrease_size(SizeType cursize,
	                              compact_string_base::size_type decsize)
	{
		assert(decsize <= cursize);
		return static_cast<SizeType>(cursize - decsize);
	}

	BOOST_NORETURN void compact_string_base::throw_xlen()
	{
		throw std::length_error("length_error");
	}

	BOOST_NOINLINE compact_string_base::value_type * compact_string_base::grow_to(size_type newsize)
	{
		using namespace compact_string_detail;
		if (newsize >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
			{
				if (newsize <= INPLACE_CAPACITY)
				{
					inplen = newsize;
					return inpbuf;
				}

				auto first = inpbuf;
				auto size = inplen;

				if (newsize <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP16;
					return newbody->buffer;
				}

				if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
					return newbody->buffer;
				}

				{
					auto * newbody = alloc_normal_body(newsize, first, size);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
					return newbody->buffer;
				}
			}

			case HEAP16:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				value_type * retptr;
				if (newsize <= HEAP16_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return oldbody->buffer;
				}
				else if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, oldbody->buffer, oldbody->length);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
					retptr = newbody->buffer;
				}
				else // HEAP
				{
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
					retptr = newbody->buffer;
				}

				delete oldbody;
				return retptr;
			}

			case HEAP32:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				if (newsize <= HEAP32_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return oldbody->buffer;
				}
				else
				{   // HEAP
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;

					delete oldbody;
					return newbody->buffer;
				}
			}

			case HEAP:
			{
				auto * oldbody = unpack<heap_normal_body>(extptr);
				if (newsize <= oldbody->capacity)
				{
					oldbody->length = newsize;
					return oldbody->buffer;
				}

				auto * newbody = alloc_normal_body_adjusted(newsize, oldbody->capacity, oldbody->buffer, oldbody->length);
				newbody->length = newsize;
				extptr = pack(newbody);

				delete oldbody;
				return newbody->buffer;
			}

			default: EXT_UNREACHABLE();
		}
	}
	
	BOOST_NOINLINE
		std::pair<compact_string_base::value_type *, compact_string_base::size_type>
		compact_string_base::grow_by(size_type size_increment)
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE:
			{
				auto newsize = increase_size(inplen, size_increment);
				if (newsize <= INPLACE_CAPACITY)
				{
					inplen = newsize;
					return {inpbuf, newsize};
				}

				auto first = inpbuf;
				auto size = inplen;

				if (newsize <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP16;
					return {newbody->buffer, newsize};
				}

				if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
					return {newbody->buffer, newsize};
				}

				{
					auto * newbody = alloc_normal_body(newsize, first, size);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
					return {newbody->buffer, newsize};
				}
			}

			case HEAP16:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				auto newsize = increase_size(oldbody->length, size_increment);
				value_type * retptr;

				if (newsize <= HEAP16_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return {oldbody->buffer, newsize};
				}
				else if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, oldbody->buffer, oldbody->length);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
					retptr = newbody->buffer;
				}
				else // HEAP
				{
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
					retptr = newbody->buffer;
				}

				delete oldbody;
				return {retptr, newsize};
			}

			case HEAP32:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				auto newsize = increase_size(oldbody->length, size_increment);

				if (newsize <= HEAP32_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return {oldbody->buffer, newsize};
				}
				else
				{   // HEAP
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;

					delete oldbody;
					return {newbody->buffer, newsize};
				}
			}

			case HEAP:
			{
				auto * oldbody = unpack<heap_normal_body>(extptr);
				auto newsize = increase_size(oldbody->length, size_increment);

				if (newsize <= oldbody->capacity)
				{
					oldbody->length = newsize;
					return {oldbody->buffer, newsize};
				}

				auto * newbody = alloc_normal_body_adjusted(newsize, oldbody->capacity, oldbody->buffer, oldbody->length);
				newbody->length = newsize;
				extptr = pack(newbody);

				delete oldbody;
				return {newbody->buffer, newsize};
			}

			default: EXT_UNREACHABLE();
		}
	}

	BOOST_NOINLINE
		std::pair<compact_string_base::value_type *, compact_string_base::size_type>
		compact_string_base::shrink_by(size_type size_decrement)
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE:
			{
				inplen = decrease_size(inplen, size_decrement);
				return {static_cast<value_type *>(inpbuf), static_cast<size_type>(inplen)};
			}

			case HEAP16:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				body->length = decrease_size(body->length, size_decrement);
				return {body->buffer, body->length};
			}

			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				body->length = decrease_size(body->length, size_decrement);
				return {body->buffer, body->length};
			}

			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				body->length = decrease_size(body->length, size_decrement);
				return {body->buffer, body->length};
			}

			default: EXT_UNREACHABLE();
		}
	}


	BOOST_NOINLINE void compact_string_base::shrink_to_fit()
	{
		using namespace compact_string_detail;

		switch (type)
		{
			case INPLACE: return;

			case HEAP16:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				auto size = body->length;
				if (size <= INPLACE_CAPACITY)
				{
					std::memcpy(body->buffer, inpbuf, size);
					inplen = size;
					type = INPLACE;
					delete body;
				}

				return;
			}

			case HEAP32:
			{
				auto * body = unpack<heap_compact_body>(extptr);
				auto size = body->length;
				if (size <= INPLACE_CAPACITY)
				{
					std::memcpy(body->buffer, inpbuf, size);
					inplen = size;
					type = INPLACE;
					delete body;
				}
				else if (size <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, body->buffer, size);
					extptr = pack(newbody);
					type = HEAP16;
					delete body;
				}

				return;
			}

			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				auto size = body->length;
				if (size <= INPLACE_CAPACITY)
				{
					std::memcpy(body->buffer, inpbuf, size);
					inplen = size;
					type = INPLACE;
					delete body;
				}
				else if (size <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, body->buffer, size);
					extptr = pack(newbody);
					type = HEAP16;
					delete body;
				}
				else if (size <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, body->buffer, size);
					extptr = pack(newbody);
					type = HEAP32;
					delete body;
				}

				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

	BOOST_NOINLINE void compact_string_base::reserve(size_type newcap)
	{
		using namespace compact_string_detail;
		if (newcap >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
			{
				if (newcap <= INPLACE_CAPACITY) return;

				auto first = inpbuf;
				auto size = inplen;

				if (newcap <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, first, size);
					extptr = pack(newbody);
					type = HEAP16;
					return;
				}

				if (newcap <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, first, size);
					extptr = pack(newbody);
					type = HEAP32;
					return;
				}

				{
					auto * newbody = alloc_normal_body(newcap, first, size);
					extptr = pack(newbody);
					type = HEAP;
					return;
				}
			}

			case HEAP16:
			{
				if (newcap <= HEAP16_CAPACITY) return;
				auto * body = unpack<heap_compact_body>(extptr);

				auto first = body->buffer;
				auto size = body->length;

				if (newcap <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, first, size);
					extptr = pack(newbody);
					type = HEAP32;
				}
				else
				{
					auto newbody = alloc_normal_body(newcap, first, size);
					extptr = pack(newbody);
					type = HEAP;
				}

				delete body;
				return;
			}

			case HEAP32:
			{
				if (newcap <= HEAP32_CAPACITY) return;

				auto * body = unpack<heap_compact_body>(extptr);
				auto * newbody = alloc_normal_body(newcap, body->buffer, body->length);
				extptr = pack(newbody);
				type = HEAP;

				delete body;
				return;
			}

			case HEAP:
			{
				auto * body = unpack<heap_normal_body>(extptr);
				auto * newbody = alloc_normal_body_adjusted(newcap, body->capacity, body->buffer, body->length);
				extptr = pack(newbody);
				type = HEAP;

				delete body;
				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

	BOOST_NOINLINE void compact_string_base::resize(size_type newsize)
	{
		using namespace compact_string_detail;
		if (newsize >= max_size()) throw_xlen();

		switch (type)
		{
			case INPLACE:
			{
				if (newsize <= INPLACE_CAPACITY)
				{
					inplen = newsize;
					return;
				}

				auto first = inpbuf;
				auto size = inplen;

				if (newsize <= HEAP16_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP16_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP16;
					return;
				}

				if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, first, size);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
					return;
				}

				{
					auto * newbody = alloc_normal_body(newsize, first, size);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
					return;
				}
			}

			case HEAP16:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				if (newsize <= HEAP16_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return;
				}
				else if (newsize <= HEAP32_CAPACITY)
				{
					auto * newbody = alloc_compact_body(HEAP32_CAPACITY, oldbody->buffer, oldbody->length);
					newbody->length = static_cast<std::uint8_t>(newsize);
					extptr = pack(newbody);
					type = HEAP32;
				}
				else // HEAP
				{
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					newbody->length = newsize;
					extptr = pack(newbody);
					type = HEAP;
				}

				delete oldbody;
				return;
			}

			case HEAP32:
			{
				auto * oldbody = unpack<heap_compact_body>(extptr);
				if (newsize <= HEAP32_CAPACITY)
				{
					oldbody->length = static_cast<std::uint8_t>(newsize);
					return;
				}
				else
				{   // HEAP
					auto * newbody = alloc_normal_body(newsize, oldbody->buffer, oldbody->length);
					extptr = pack(newbody);
					type = HEAP;
				}

				delete oldbody;
				return;
			}

			case HEAP:
			{
				auto * oldbody = unpack<heap_normal_body>(extptr);
				if (newsize <= oldbody->capacity)
				{
					oldbody->length = newsize;
					return;
				}

				auto * newbody = alloc_normal_body_adjusted(newsize, oldbody->capacity, oldbody->buffer, oldbody->length);
				extptr = pack(newbody);

				delete oldbody;
				return;
			}

			default: EXT_UNREACHABLE();
		}
	}

	/************************************************************************/
	/*                  ctors/dtors                                         */
	/************************************************************************/
	compact_string_base::compact_string_base(const compact_string_base & other)
	{
		using namespace compact_string_detail;
		
		type = other.type;
		switch (type)
		{
			case INPLACE:
				base_type::operator =(other);
				return;

			case HEAP16:
				base_type::extptr = pack(alloc_copy(unpack<heap_compact_body>(other.base_type::extptr), HEAP16_CAPACITY));
				return;

			case HEAP32:
				base_type::extptr = pack(alloc_copy(unpack<heap_compact_body>(other.base_type::extptr), HEAP32_CAPACITY));
				return;

			case HEAP:
				base_type::extptr = pack(alloc_copy(unpack<heap_normal_body>(other.base_type::extptr)));
				return;

			default: EXT_UNREACHABLE();
		}
	}

	compact_string_base & compact_string_base::operator =(const compact_string_base & other)
	{
		if (this != &other)
		{
			this->~compact_string_base();
			new (this) compact_string_base(other);
		}

		return *this;
	}

	compact_string_base::~compact_string_base() BOOST_NOEXCEPT
	{
		using namespace compact_string_detail;
		switch (type)
		{
			case INPLACE:   return;

			case HEAP16:
			case HEAP32:
				delete unpack<heap_compact_body>(base_type::extptr);
				return;

			case HEAP:
				delete unpack<heap_normal_body>(base_type::extptr);
				return;

			default: EXT_UNREACHABLE();
		}
	}

} // namespace ext
