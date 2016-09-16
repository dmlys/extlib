#pragma once
#include <cstddef>
#include <limits>

#include <boost/config.hpp>
#include <ext/intrusive_ptr.hpp>

namespace ext
{
	class cow_string_body
	{
	private:
		typedef cow_string_body      self_type;

		struct heap_body
		{
			unsigned refs = 0;
			std::size_t size;
			std::size_t capacity;
			char buffer[1];
		};

		friend inline      void intrusive_ptr_add_ref(heap_body * ptr) noexcept   { ++ptr->refs; }
		friend inline      void intrusive_ptr_release(heap_body * ptr) noexcept   { if (--ptr->refs == 0) delete ptr; }
		friend inline  unsigned intrusive_ptr_use_count(const heap_body * ptr) noexcept { return ptr->refs; }
		friend inline heap_body * intrusive_ptr_default(const heap_body * ptr) noexcept { intrusive_ptr_add_ref(&ms_shared_null); return &ms_shared_null; }
		friend void intrusive_ptr_clone(const heap_body * ptr, heap_body * & dest);

	public:
		typedef char value_type;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		typedef std::pair<value_type *, value_type *>              range_type;
		typedef std::pair<const value_type *, const value_type *>  const_range_type;

	private:
		ext::intrusive_cow_ptr<heap_body> m_body;
		static heap_body ms_shared_null;

	private:
		static heap_body * alloc_body(std::nothrow_t, size_type cap);
		static heap_body * alloc_body(size_type cap);
		static heap_body * alloc_body_adjusted(const heap_body & oldbody, size_type newcap);
		static heap_body   init_shared_null();

		value_type * mutable_buffer() const noexcept;
		value_type * mutable_bufend() const noexcept;

	public:
		      value_type * data()       noexcept { return m_body->buffer; }
		const value_type * data() const noexcept { return m_body->buffer; }

		      value_type * data_end()       noexcept;
		const value_type * data_end() const noexcept;

		      range_type range()       noexcept;
		const_range_type range() const noexcept;

		static size_type max_size() noexcept { return (std::numeric_limits<size_type>::max)(); }
		size_type capacity()  const noexcept { return m_body->capacity; }
		size_type size()      const noexcept { return m_body->size;  }
		bool empty()          const noexcept { return size() == 0; }

		unsigned use_count()  const noexcept { return m_body.use_count(); }

		const value_type * c_str() const noexcept { return data(); }
		BOOST_NORETURN static void throw_xlen();

	public:
		void resize(size_type newsize);
		void reserve(size_type newcap);
		void shrink_to_fit();

	protected:
		value_type * grow_to(size_type newsize);
		std::pair<value_type *, size_type> grow_by(size_type size_increment);
		std::pair<value_type *, size_type> shrink_by(size_type size_decrement);

		inline static void set_eos(value_type * pos) { *pos = 0; }

	public:
		cow_string_body() = default;
		~cow_string_body() = default;

		//cow_string_body(const self_type &) = default;
		//cow_string_body(self_type &&) = default;
		//cow_string_body & operator =(const self_type &) = default;
		//cow_string_body & operator =(self_type &&) = default;

		friend void swap(self_type & s1, self_type & s2) { swap(s1, s2); }
	};

	inline auto cow_string_body::data_end() noexcept -> value_type *
	{
		auto & body = *m_body;
		return body.buffer + body.size;
	}

	inline auto cow_string_body::data_end() const noexcept -> const value_type * 
	{
		auto & body = *m_body;
		return body.buffer + body.size;
	}

	inline auto cow_string_body::range() noexcept -> range_type
	{
		auto & body = *m_body;
		return {body.buffer, body.buffer + body.size};
	}

	inline auto cow_string_body::range() const noexcept -> const_range_type
	{
		auto & body = *m_body;
		return {body.buffer, body.buffer + body.size};
	}
}
