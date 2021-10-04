#pragma once
#include <cassert>
#include <utility> // for std::exchange
#include <boost/operators.hpp>
#include <ext/noaddref.hpp>

namespace ext
{
	/// similar to unique_ptr, but manges object pointed by numeric handle.
	/// HandleTraits should provide 2 static functions:
	/// * void close(HandleType handle) noexcept
	///        closes handle
	/// * HandleType emptyval() noexcept
	///        returns empty/invalid handle value, this is default value, analog for nullptr for unique_ptr
	template <class HandleType, class HandleTraits, class TagType = void>
	class unique_handle :
		boost::totally_ordered<unique_handle<HandleType, HandleTraits, TagType>, HandleType,
		  boost::totally_ordered1<unique_handle<HandleType, HandleTraits, TagType>,
		    HandleTraits> >
	{
		using self_type = unique_handle;

	public:
		using handle_traits = HandleTraits;
		using handle_type   = HandleType;
		using tag_type      = TagType;

	private:
		handle_type m_handle = emptyval();

	private:
		static handle_type emptyval() noexcept { return handle_traits::emptyval(); }

	public:
		// releases the ownership of the managed object if any.
		// get() returns defval after the call. Semanticly same as unique_ptr::release
		auto release() noexcept { return std::exchange(m_handle, emptyval()); }

		// reset returns whatever intrusive_ptr_release returns(including void)
		void reset(handle_type handle) noexcept { handle_traits::close(m_handle); m_handle = handle; }
		void reset()                   noexcept { return reset(emptyval()); }

		handle_type get() const noexcept { return m_handle; }

		//explicit operator handle_type() const noexcept { return m_handle; }
		explicit operator bool() const noexcept { return m_handle != emptyval(); }

	public:
		bool operator  <(const unique_handle & ptr) const noexcept { return m_handle  < ptr.m_handle; }
		bool operator ==(const unique_handle & ptr) const noexcept { return m_handle == ptr.m_handle; }

		bool operator  <(handle_type handle) const noexcept { return m_handle  < handle; }
		bool operator ==(handle_type handle) const noexcept { return m_handle == handle; }

	public:
		explicit unique_handle(handle_type handle) noexcept : m_handle(handle) { }

		unique_handle()  noexcept = default;
		~unique_handle() noexcept { handle_traits::close(m_handle); }

		unique_handle(self_type && op) noexcept : m_handle(op.release()) {}
		unique_handle & operator =(self_type && op) noexcept { if (this != &op) reset(op.release());  return *this; }

		friend void swap(unique_handle & p1, unique_handle & p2) noexcept { std::swap(p1.m_handle, p2.m_handle); }
	};
}
