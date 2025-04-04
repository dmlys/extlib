#pragma once
#include <cassert>
#include <cstddef> // for nullptr_t
#include <utility> // for std::declval, std::exchange
#include <type_traits>
#include <atomic>
#include <boost/operators.hpp>
#include <ext/noaddref.hpp>

namespace ext
{	
	/// ADL pointer traits for intrusive_ptr/intrusive_cow_ptr, pointer is managed via functions:
	/// * intrusive_ptr_add_ref(value_type *) -> void
	/// * intrusive_ptr_release(value_type *) -> void
	/// * intrusive_ptr_use_count(const value_type *) -> counter_type, optional
	/// additionally for intrusive_cow_ptr
	/// * intrusive_ptr_default(const value_type *) -> convertible to   value_type *
	/// * intrusive_ptr_clone(const value_type *, value_type *&) -> void
	/// 
	/// all functions, even those who actually not need it,
	/// take pointer as argument, so functions can be provided on ADL basis.
	/// 
	/// default constructed intrusive_cow_ptr holds a value returned by intrusive_ptr_default,
	/// which is typically nullptr, but can be some shared empty val.
	/// 
	/// Functions description:
	/// * void intrusive_ptr_add_ref(value_type * ptr)
	///        increase reference counter
	///        typical implementation would be:
	///        ++ptr->refs
	/// 
	/// * void intrusive_ptr_release(value_type * ptr)
	///        decreases reference counter and deletes object if counter reaches 0
	///        typical implementation would be:
	///        if (--ptr->refs == 0)
	///           delete ptr;
	/// 
	/// * auto intrusive_ptr_use_count(const value_type * ptr) -> ${arithmetic type}
	///        returns current refs. for nullptr should return 0
	///        typical implementation would be:
	///        return ptr->refs;
	/// 
	/// additionally for intrusive_cow_ptr
	/// * auto intrusive_ptr_default(const value_type * unused) -> value_type */std::nullptr_t
	/// 
	///        returns pointer for a default constructed object.
	///        if it returns pointer to shared empty object,
	///        reference counter must be increased as if by intrusive_ptr_add_ref.
	///
	///        typical implementation would be: return nullptr;
	///        or for shared empty:  intrusive_ptr_add_ref(&g_global_shared); return &g_global_shared;
	///        
	/// * void intrusive_ptr_clone(const value_type * ptr, value_type * & dest)
	///        returns copy of object pointed by ptr into dest,
	///        copy must have increased reference count as if by intrusive_ptr_add_ref.
	///        
	///        typical implementation would be:
	///       	dest = new value_type(*ptr);
	///       	copy->refs = 1;
	/// 
	class adl_intrusive_pointer_traits
	{
	public:
		template <class Type> static inline auto use_count(const Type * ptr) noexcept { return ptr ? intrusive_ptr_use_count(ptr) : 0; }
		template <class Type> static inline auto addref(Type * ptr)          noexcept { if (ptr) intrusive_ptr_add_ref(ptr); }
		template <class Type> static inline auto subref(Type * ptr)          noexcept { if (ptr) intrusive_ptr_release(ptr); }
		
		template <class Type> static inline auto defval(const Type * ptr)               noexcept { return intrusive_ptr_default(ptr); }
		template <class Type> static inline auto clone(const Type * ptr, Type * & dest) noexcept { return intrusive_ptr_clone(ptr, dest); }
	};
	
	/// smart pointer for intrusive types.
	/// see also adl_intrusive_pointer_traits, intrusive_atomic_counter, intrusive_plain_counter helper classes.
	template <class Type, class PointerTraits = adl_intrusive_pointer_traits>
	class intrusive_ptr :
		boost::totally_ordered<intrusive_ptr<Type, PointerTraits>, const Type *,
		  boost::totally_ordered1<intrusive_ptr<Type, PointerTraits>, 
		    PointerTraits> >
	{
		using self_type = intrusive_ptr;

	public:
		using pointer_traits = PointerTraits;
		
		using value_type = Type;
		using pointer    = Type *;
		using referecne  = Type &;

	private:
		value_type * m_ptr = nullptr;

	public:
		auto use_count() const noexcept { return pointer_traits::use_count(m_ptr); }
		void addref()          noexcept { return pointer_traits::addref(m_ptr); }
		void subref()          noexcept { return pointer_traits::subref(m_ptr); }

		// releases the ownership of the managed object if any.
		// get() returns nullptr after the call. Same as unique_ptr::release
		auto release() noexcept { return std::exchange(m_ptr, nullptr); }


		void reset(value_type * ptr)                noexcept;
		void reset(value_type * ptr, noaddref_type) noexcept;
		void reset(value_type * ptr, bool add_ref)  noexcept;
		void reset()                                noexcept { return reset(nullptr); }

		value_type * get() const noexcept { return m_ptr; }
		value_type & operator *() const noexcept { return *get(); }
		value_type * operator ->() const noexcept { return get(); }

		explicit operator bool() const noexcept { return m_ptr != nullptr; }

	public:
		bool operator  <(const intrusive_ptr & ptr) const noexcept { return m_ptr < ptr.m_ptr; }
		bool operator ==(const intrusive_ptr & ptr) const noexcept { return m_ptr == ptr.m_ptr; }

		bool operator  <(const value_type * ptr) const noexcept { return m_ptr < ptr; }
		bool operator ==(const value_type * ptr) const noexcept { return m_ptr == ptr; }

	public:
		explicit intrusive_ptr(value_type * ptr)                noexcept : m_ptr(ptr) { addref(); }
		explicit intrusive_ptr(value_type * ptr, noaddref_type) noexcept : m_ptr(ptr) { }
		explicit intrusive_ptr(value_type * ptr, bool add_ref)  noexcept : m_ptr(ptr) { if (add_ref) addref(); }

		intrusive_ptr()  noexcept : m_ptr(nullptr) {};
		~intrusive_ptr() noexcept { subref(); }

		intrusive_ptr(const self_type & op) noexcept : m_ptr(op.m_ptr) { addref(); }
		intrusive_ptr(self_type && op)      noexcept : m_ptr(op.release()) {}

		intrusive_ptr & operator =(const self_type & op) noexcept { if (this != &op) reset(op.m_ptr);                return *this; }
		intrusive_ptr & operator =(self_type && op)      noexcept { if (this != &op) reset(op.release(), noaddref);  return *this; }

		friend void swap(intrusive_ptr & p1, intrusive_ptr & p2) noexcept { std::swap(p1.m_ptr, p2.m_ptr); }

		template <class Other, class = typename std::enable_if_t<std::is_convertible<Other *, value_type *>::value>>
		intrusive_ptr(const intrusive_ptr<Other, PointerTraits> & other) noexcept
			: m_ptr(other.get())
		{
			addref();
		}

		// for intrusive_ptr p = nullptr;
		intrusive_ptr(std::nullptr_t) : m_ptr(nullptr) {};
		intrusive_ptr & operator =(std::nullptr_t) noexcept { subref(); m_ptr = nullptr; return *this; }
	};

	template <class Type, class PointerTraits>
	void intrusive_ptr<Type, PointerTraits>::reset(value_type * ptr) noexcept
	{
		subref();
		m_ptr = ptr;
		addref();
	}

	template <class Type, class PointerTraits>
	void intrusive_ptr<Type, PointerTraits>::reset(value_type * ptr, noaddref_type) noexcept
	{
		subref();
		m_ptr = ptr;
	}

	template <class Type, class PointerTraits>
	void intrusive_ptr<Type, PointerTraits>::reset(value_type * ptr, bool add_ref) noexcept
	{
		subref();
		m_ptr = ptr;
		if (add_ref) addref();
	}

	template <class Type, class PointerTraits = adl_intrusive_pointer_traits, class ... Args>
	intrusive_ptr<Type, PointerTraits> make_intrusive(Args && ... args)
	{
		return intrusive_ptr<Type, PointerTraits>(new Type(std::forward<Args>(args)...), ext::noaddref);
	}

	template <class Type, class PointerTraits>
	inline const Type * get_pointer(const intrusive_ptr<Type, PointerTraits> & ptr)
	{
		return ptr.get();
	}

	template <class Type, class PointerTraits>
	inline Type * get_pointer(intrusive_ptr<Type, PointerTraits> & ptr)
	{
		return ptr.get();
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_ptr<DestType, PointerTraits> const_pointer_cast(const intrusive_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_ptr<DestType, PointerTraits> {const_cast<DestType *>(ptr.get())};
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_ptr<DestType, PointerTraits> static_pointer_cast(const intrusive_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_ptr<DestType, PointerTraits> {static_cast<DestType *>(ptr.get())};
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_ptr<DestType, PointerTraits> dynamic_pointer_cast(const intrusive_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_ptr<DestType, PointerTraits> {dynamic_cast<DestType *>(ptr.get())};
	}



	/// smart pointer for intrusive copy-on-write(cow) types, functionally extends intrusive_ptr.
	/// see also adl_intrusive_pointer_traits, intrusive_atomic_counter, intrusive_plain_counter helper classes.
	template <class Type, class PointerTraits = adl_intrusive_pointer_traits>
	class intrusive_cow_ptr :
		boost::totally_ordered<intrusive_cow_ptr<Type, PointerTraits>, const Type *,
		  boost::totally_ordered1<intrusive_cow_ptr<Type, PointerTraits>, 
		    PointerTraits> >
	{
		using self_type = intrusive_cow_ptr;

	public:
		using pointer_traits = PointerTraits;
		
		using value_type = Type;
		using pointer    = Type *;
		using referecne  = Type &;

	private:
		value_type * m_ptr = defval();

	private:
		static value_type * defval() noexcept { return pointer_traits::defval(static_cast<const value_type *>(nullptr)); }

	public:
		auto use_count() const noexcept { return pointer_traits::use_count(m_ptr); }
		void addref()          noexcept { return pointer_traits::addref(m_ptr); }
		void subref()          noexcept { return pointer_traits::subref(m_ptr); }

		// releases the ownership of the managed object if any.
		// get() returns nullptr after the call. Same as unique_ptr::release
		auto release() noexcept { return std::exchange(m_ptr, defval()); }


		void reset(value_type * ptr)                noexcept;
		void reset(value_type * ptr, noaddref_type) noexcept;
		void reset(value_type * ptr, bool add_ref)  noexcept;
		void reset()                                noexcept { return reset(defval()); }
		void detach();

		// can be dangerous, does not detach, consider use of get, * or ->
		value_type * get_ptr() const noexcept { return m_ptr; }

		      value_type * get()                { detach(); return m_ptr; }
		const value_type * get() const noexcept { return m_ptr; }

		      value_type & operator *()                { return *get(); }
		const value_type & operator *() const noexcept { return *get(); }

		      value_type * operator ->()                { return get(); }
		const value_type * operator ->() const noexcept { return get(); }

		explicit operator bool() const noexcept { return m_ptr != nullptr; }

	public:
		bool operator  <(const intrusive_cow_ptr & ptr) const noexcept { return m_ptr < ptr.m_ptr; }
		bool operator ==(const intrusive_cow_ptr & ptr) const noexcept { return m_ptr == ptr.m_ptr; }

		bool operator  <(const value_type * ptr) const noexcept { return m_ptr < ptr; }
		bool operator ==(const value_type * ptr) const noexcept { return m_ptr == ptr; }

	public:
		explicit intrusive_cow_ptr(value_type * ptr)                noexcept : m_ptr(ptr) { addref(); }
		explicit intrusive_cow_ptr(value_type * ptr, noaddref_type) noexcept : m_ptr(ptr) { }
		explicit intrusive_cow_ptr(value_type * ptr, bool add_ref)  noexcept : m_ptr(ptr) { if (add_ref) addref(); }

		intrusive_cow_ptr()  noexcept : m_ptr(defval()) {};
		~intrusive_cow_ptr() noexcept { subref(); }

		intrusive_cow_ptr(const self_type & op) noexcept : m_ptr(op.m_ptr) { addref(); }
		intrusive_cow_ptr(self_type && op)      noexcept : m_ptr(op.release()) {}

		intrusive_cow_ptr & operator =(const self_type & op) noexcept { if (this != &op) reset(op.m_ptr);                return *this; }
		intrusive_cow_ptr & operator =(self_type && op)      noexcept { if (this != &op) reset(op.release(), noaddref);  return *this; }

		friend void swap(intrusive_cow_ptr & p1, intrusive_cow_ptr & p2) noexcept { std::swap(p1.m_ptr, p2.m_ptr); }

		template <class Other, class = typename std::enable_if_t<std::is_convertible<Other *, value_type *>::value>>
		intrusive_cow_ptr(const intrusive_cow_ptr<Other, PointerTraits> & other) noexcept
			: m_ptr(other.get_ptr())
		{ addref(); }

		// for instrusive_cow_ptr p = nullptr;
		intrusive_cow_ptr(std::nullptr_t) noexcept : m_ptr(nullptr) {};
		intrusive_cow_ptr & operator =(std::nullptr_t) noexcept { subref(); m_ptr = nullptr; return *this; }
	};

	template <class Type, class PointerTraits>
	inline void intrusive_cow_ptr<Type, PointerTraits>::detach()
	{
		// we are the only one, detach is not needed
		if (use_count() <= 1) return;

		// make a copy
		value_type * copy;
		pointer_traits::clone(m_ptr, copy);
		pointer_traits::subref(m_ptr);
		m_ptr = copy;
	}

	template <class Type, class PointerTraits>
	inline void intrusive_cow_ptr<Type, PointerTraits>::reset(value_type * ptr) noexcept
	{
		subref();
		m_ptr = ptr;
		addref();
	}

	template <class Type, class PointerTraits>
	inline void intrusive_cow_ptr<Type, PointerTraits>::reset(value_type * ptr, noaddref_type) noexcept
	{
		subref();
		m_ptr = ptr;
	}

	template <class Type, class PointerTraits>
	inline void intrusive_cow_ptr<Type, PointerTraits>::reset(value_type * ptr, bool add_ref) noexcept
	{
		subref();
		m_ptr = ptr;
		if (add_ref) addref();
	}
	
	template <class Type, class PointerTraits>
	inline const Type * get_pointer(const intrusive_cow_ptr<Type, PointerTraits> & ptr)
	{
		return ptr.get();
	}

	template <class Type, class PointerTraits>
	inline Type * get_pointer(intrusive_cow_ptr<Type, PointerTraits> & ptr)
	{
		return ptr.get();
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_cow_ptr<DestType, PointerTraits> const_pointer_cast(const intrusive_cow_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_cow_ptr<DestType, PointerTraits> {const_cast<DestType *>(ptr.get_ptr())};
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_cow_ptr<DestType, PointerTraits> static_pointer_cast(const intrusive_cow_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_cow_ptr<DestType, PointerTraits> {static_cast<DestType *>(ptr.get_ptr())};
	}

	template <class DestType, class Type, class PointerTraits>
	inline intrusive_cow_ptr<DestType, PointerTraits> dynamic_pointer_cast(const intrusive_cow_ptr<Type, PointerTraits> & ptr)
	{
		return intrusive_cow_ptr<DestType, PointerTraits> {dynamic_cast<DestType *>(ptr.get_ptr())};
	}



	/// intrusive_atomic_counter implements atomic thread safe reference counter
	/// for a derived user's class that is intended to be used with intrusive_ptr/intrusive_cow_ptr.
	/// For complex hierarchies do not forget virtual destructor and probably virtual clone function
	template <class Derived>
	class intrusive_atomic_counter
	{
	protected:
		std::atomic<unsigned> m_refs = {1};

	protected:
		unsigned counter_addref() noexcept;
		unsigned counter_release() noexcept;
		unsigned counter_usecount() const noexcept;

	public:
		intrusive_atomic_counter() = default;
		intrusive_atomic_counter(intrusive_atomic_counter &&) = default;
		intrusive_atomic_counter(const intrusive_atomic_counter &) noexcept {};

		intrusive_atomic_counter & operator =(intrusive_atomic_counter &&) = default;
		intrusive_atomic_counter & operator =(const intrusive_atomic_counter &) noexcept { return *this; }


		template <class Type> friend       unsigned intrusive_ptr_add_ref(intrusive_atomic_counter<Type> * ptr) noexcept;
		template <class Type> friend       unsigned intrusive_ptr_release(intrusive_atomic_counter<Type> * ptr) noexcept;
		template <class Type> friend       unsigned intrusive_ptr_use_count(const intrusive_atomic_counter<Type> * ptr) noexcept;
		template <class Type> friend std::nullptr_t intrusive_ptr_default(const intrusive_atomic_counter<Type> * ptr) noexcept;
		
		template <class Type, class DestType>
		friend void intrusive_ptr_clone(const intrusive_atomic_counter<Type> * ptr, DestType * & dest);
	};

	template <class Derived>
	inline unsigned intrusive_atomic_counter<Derived>::counter_addref() noexcept
	{
		return m_refs.fetch_add(1, std::memory_order_relaxed);
	}

	template <class Derived>
	inline unsigned intrusive_atomic_counter<Derived>::counter_release() noexcept
	{
		auto ref = m_refs.fetch_sub(1, std::memory_order_release);
		if (ref == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);
			delete static_cast<Derived *>(this);
		}

		return --ref;
	}

	template <class Derived>
	inline unsigned intrusive_atomic_counter<Derived>::counter_usecount() const noexcept
	{
		return m_refs.load(std::memory_order_relaxed);
	}

	template <class Type>
	inline unsigned intrusive_ptr_add_ref(intrusive_atomic_counter<Type> * ptr) noexcept
	{
		return ptr->counter_addref();
	}

	template <class Type>
	inline unsigned intrusive_ptr_release(intrusive_atomic_counter<Type> * ptr) noexcept
	{
		return ptr->counter_release();
	}

	template <class Type>
	inline unsigned intrusive_ptr_use_count(const intrusive_atomic_counter<Type> * ptr) noexcept
	{
		return ptr->counter_usecount();
	}

	template <class Type>
	inline std::nullptr_t intrusive_ptr_default(const intrusive_atomic_counter<Type> *) noexcept
	{
		return nullptr;
	}

	template <class Type, class DestType>
	inline void intrusive_ptr_clone(const intrusive_atomic_counter<Type> * ptr, DestType * & dest)
	{
		assert(ptr);
		dest = new Type(*static_cast<const Type *>(ptr));
		dest->m_refs.store(1u, std::memory_order_relaxed);
	}


	/// intrusive_plain_counter implements thread unsafe simple reference counter
	/// for a derived user's class that is intended to be used with intrusive_ptr/intrusive_cow_ptr.
	/// For complex hierarchies do not forget virtual destructor and probably virtual clone function
	template <class Derived>
	class intrusive_plain_counter
	{
	protected:
		unsigned m_refs = 1;

	protected:
		unsigned counter_addref() noexcept;
		unsigned counter_release() noexcept;
		unsigned counter_usecount() const noexcept;

	public:
		intrusive_plain_counter() = default;
		intrusive_plain_counter(intrusive_plain_counter &&) = default;
		intrusive_plain_counter(const intrusive_plain_counter &) noexcept {};

		intrusive_plain_counter & operator =(intrusive_plain_counter &&) = default;
		intrusive_plain_counter & operator =(const intrusive_plain_counter &) noexcept { return *this; }


		template <class Type> friend       unsigned intrusive_ptr_add_ref(intrusive_plain_counter<Type> * ptr) noexcept;
		template <class Type> friend       unsigned intrusive_ptr_release(intrusive_plain_counter<Type> * ptr) noexcept;
		template <class Type> friend       unsigned intrusive_ptr_use_count(const intrusive_plain_counter<Type> * ptr) noexcept;
		template <class Type> friend std::nullptr_t intrusive_ptr_default(const intrusive_plain_counter<Type> * ptr) noexcept;
		
		template <class Type, class DestType>
		friend void intrusive_ptr_clone(const intrusive_plain_counter<Type> * ptr, DestType * & dest);
	};

	template <class Derived>
	inline unsigned intrusive_plain_counter<Derived>::counter_addref() noexcept
	{
		return m_refs++;
	}

	template <class Derived>
	inline unsigned intrusive_plain_counter<Derived>::counter_release() noexcept
	{
		if (--m_refs == 0)
			delete static_cast<Derived *>(this);

		return m_refs;
	}

	template <class Derived>
	inline unsigned intrusive_plain_counter<Derived>::counter_usecount() const noexcept
	{
		return m_refs;
	}

	template <class Type>
	inline unsigned intrusive_ptr_add_ref(intrusive_plain_counter<Type> * ptr) noexcept
	{
		return ptr->counter_addref();
	}

	template <class Type>
	inline unsigned intrusive_ptr_release(intrusive_plain_counter<Type> * ptr) noexcept
	{
		return ptr->counter_release();
	}

	template <class Type>
	inline unsigned intrusive_ptr_use_count(const intrusive_plain_counter<Type> * ptr) noexcept
	{
		return ptr->counter_usecount();
	}

	template <class Type>
	inline std::nullptr_t intrusive_ptr_default(const intrusive_plain_counter<Type> *) noexcept
	{
		return nullptr;
	}

	template <class Type, class DestType>
	inline void intrusive_ptr_clone(const intrusive_plain_counter<Type> * ptr, DestType * & dest)
	{
		assert(ptr);
		dest = new Type(*static_cast<const Type *>(ptr));
		dest->m_refs = 1u;
	}
}
