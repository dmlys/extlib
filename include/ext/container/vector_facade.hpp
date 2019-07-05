#pragma once
// author: Dmitry Lysachenko
// date: Sunday 28 Sun 2019
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <type_traits>
#include <initializer_list>
#include <utility>   // for std::exchange
#include <tuple>     // for std::tie
#include <iterator>  // for reverse_iterator
#include <algorithm>

#include <ext/type_traits.hpp>
#include <ext/config.hpp>
#include <ext/container/uninitialized_algo.hpp>
#include <ext/container/container_iterator.hpp>

namespace ext::container
{
	/// A Policy-based std::vector facade.
	///
	/// It provides all additional operation like begin, end, insert, erase, etc
	/// via basic operations of storage:
	///  * Storage holds pointers and allocator
	///  * vector_facade does and controls both allocation and destruction of elements.
	///
	/// storage interface:
	///  typedefs:
	///    * value_type
	///    * size_type
	///    * allocator_type
	///  methods(all are noexcept, except maybe get_allocator, set_allocator):
	///    * get_first -> pointer
	///    * get_last  -> pointer
	///    * get_capacity -> size_type
	///    * get_size     -> size_type
	///
	///    * get_storage_pointers -> tuple-like<pointer, pointer, pointer>
	///    * set_storage_pointers(tuple-like<pointer, pointer, pointer>)
	///       storage_pointers are: first, last, end; where [first, last) - current constructed elements; [first, end) - allocated memory
	///
	///    * set_allocator
	///    * get_allocator
	///
	///    * constructor from allocator
	///    * move constructor should null current pointers after passing them into moved object
	///    * destructor should not delete or free anything, vector_facade will do this
	///
	template <class storage>
	class vector_facade
	{
		typedef storage           storage_type;
		typedef storage_type      base_type;
		typedef vector_facade     self_type;

	public:
		using value_type      = typename storage_type::value_type;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;

		using pointer         = value_type *;
		using const_pointer   = const value_type *;
		using reference       = value_type &;
		using const_reference = const value_type &;

		//using iterator       = pointer;
		//using const_iterator = const_pointer;
		using iterator = container_iterator<pointer, vector_facade>;
		using const_iterator = container_iterator<const_pointer, vector_facade>;

		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	public:
		using allocator_type = typename storage_type::allocator_type;
		using allocator_traits = std::allocator_traits<allocator_type>;

	private:
		class auto_set_storage
		{
			self_type * parent;
			pointer * newfirst, * newlast, * newend;

		public:
			auto_set_storage(self_type * parent, pointer & newfirst, pointer & newlast, pointer & newend)
			    : parent(parent), newfirst(&newfirst), newlast(&newlast), newend(&newend) {}
			~auto_set_storage() { parent->m_storage.set_storage_pointers({*newfirst, *newlast, *newend}); }

			auto_set_storage(auto_set_storage &&) = delete;
			auto_set_storage & operator =(auto_set_storage &&) = delete;
		};

	private:
		storage_type m_storage;

	private:
		static constexpr bool use_relloc() { return std::is_nothrow_move_constructible_v<value_type> or not std::is_copy_constructible_v<value_type>; }

		static constexpr bool is_nothrow_move_constructable() { return typename allocator_traits::is_always_equal(); }
		static constexpr bool is_nothrow_move_assignable()
		{
			return typename allocator_traits::propagate_on_container_move_assignment()
			    or typename allocator_traits::is_always_equal();
		}

	private:
		[[noreturn]] static void throw_xpos() { throw std::out_of_range("out_of_range"); }
		[[noreturn]] static void throw_xlen() { throw std::length_error("length_error"); }

		static size_type check_newsize(size_type newsize);
		static size_type check_newsize(size_type cursize, size_type increment);

		//static constexpr iterator  make_iterator(const_iterator it) noexcept { return const_cast<pointer>(it); }
		//static constexpr pointer extract_pointer(const_iterator it) noexcept { return const_cast<pointer>(it); }
		//static constexpr pointer extract_pointer(      iterator it) noexcept { return it; }

		static constexpr iterator  make_iterator(const_iterator it) noexcept { return iterator(const_cast<pointer>(it.base())); }
		static constexpr pointer extract_pointer(const_iterator it) noexcept { return const_cast<pointer>(it.base()); }
		static constexpr pointer extract_pointer(      iterator it) noexcept { return it.base(); }

	private:
		auto allocate(allocator_type & alloc, size_type cap, const_pointer hint) -> std::pair<pointer, pointer>;
		void deallocate(allocator_type & alloc, pointer ptr, size_type cap) noexcept;
		auto allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::pair<pointer, pointer>;

		auto checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::pair<pointer, pointer>;
		auto checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::pair<pointer, pointer>;

	private:
		template <class Iterator>
		void do_range_assign(Iterator first, Iterator last, std::input_iterator_tag);
		template <class Iterator>
		void do_range_assign(Iterator first, Iterator last, std::forward_iterator_tag);

		template <class Iterator>
		iterator do_range_insert(const_iterator pos, Iterator first, Iterator last, std::input_iterator_tag);
		template <class Iterator>
		iterator do_range_insert(const_iterator pos, Iterator first, Iterator last, std::forward_iterator_tag);

	public:
		static constexpr size_type max_size() noexcept { return (std::numeric_limits<size_type>::max)(); }

		      pointer data()       noexcept { return m_storage.get_first(); }
		const_pointer data() const noexcept { return m_storage.get_first(); }

		size_type capacity() const noexcept { return m_storage.get_capacity(); }
		size_type size()     const noexcept { return m_storage.get_size(); }

		bool empty() const noexcept { return size() == 0; }

		void resize(size_type newsize);
		void resize(size_type newsize, const value_type & val);
		void reserve(size_type newcapacity);
		void shrink_to_fit();

		allocator_type get_allocator() const { return m_storage.get_allocator(); }

	public: // element access
		reference       at(size_type pos);
		const_reference at(size_type pos) const;

		reference       operator [](size_type pos)       noexcept { return *(data() + pos); }
		const_reference operator [](size_type pos) const noexcept { return *(data() + pos); }

		reference       front()       noexcept { return *data(); }
		const_reference front() const noexcept { return *data(); }

		reference       back()       noexcept { return *(end() - 1); }
		const_reference back() const noexcept { return *(end() - 1); }

	public:
		void pop_back();
		void push_back(const value_type & ch);
		void push_back(value_type && ch);

		template <class ... Args>
		void emplace_back(Args && ... args);

		template <class ... Args>
		iterator emplace(const_iterator pos, Args && ... args);

		void clear() noexcept { resize(0); }

	public: // iterators [done]
		iterator begin()             noexcept   { return iterator(m_storage.get_first()); }
		iterator end()               noexcept   { return iterator(m_storage.get_last());  }
		const_iterator begin() const noexcept   { return const_iterator(m_storage.get_first()); }
		const_iterator end()   const noexcept	{ return const_iterator(m_storage.get_last());  }

		const_iterator cbegin() const noexcept  { return begin(); }
		const_iterator cend()   const noexcept  { return end();   }

		reverse_iterator rbegin()             noexcept     { return reverse_iterator(end()); }
		reverse_iterator rend()               noexcept     { return reverse_iterator(begin()); }
		const_reverse_iterator rbegin() const noexcept     { return const_reverse_iterator(end()); }
		const_reverse_iterator rend()   const noexcept     { return const_reverse_iterator(begin()); }

		const_reverse_iterator crbegin() const noexcept    { return const_reverse_iterator(cend()); }
		const_reverse_iterator crend()   const noexcept    { return const_reverse_iterator(cbegin()); }

	public:
		void assign(size_type count, const value_type & val);

		template <class Iterator>
		void assign(Iterator first, Iterator last)
		{ return do_range_assign(first, last, typename std::iterator_traits<Iterator>::iterator_category()); }

		void assign(std::initializer_list<value_type> ilist) { return assign(ilist.begin(), ilist.end()); }

	public:
		iterator insert(const_iterator pos, const value_type & value) { return emplace(pos, value); }
		iterator insert(const_iterator pos, value_type && value) { return emplace(pos, std::move(value)); }
		iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) { return insert(pos, ilist.begin(), ilist.end()); }

		iterator insert(const_iterator pos, size_type count, const value_type & value);

		template<class Iterator>
		iterator insert(const_iterator pos, Iterator first, Iterator last)
		{ return do_range_insert(pos, first, last, typename std::iterator_traits<Iterator>::iterator_category()); }

	public:
		iterator erase(const_iterator pos);
		iterator erase(const_iterator first, const_iterator last);

	public:
		vector_facade()  noexcept = default;
		~vector_facade() noexcept;

		explicit vector_facade(const allocator_type & alloc) noexcept;
		explicit vector_facade(size_type count, const allocator_type & alloc = allocator_type())
		    : vector_facade(alloc)  { resize(count); }
		explicit vector_facade(size_type count, const value_type & val, const allocator_type & alloc = allocator_type())
		    : vector_facade(alloc) { resize(count, val); }

		template <class Iterator>
		explicit vector_facade(Iterator first, Iterator last, const allocator_type & alloc = allocator_type())
		    : vector_facade(alloc) { assign(first, last); }

		vector_facade(std::initializer_list<value_type> ilist, const allocator_type & alloc = allocator_type())
		    : vector_facade(alloc) { assign(ilist); }

		vector_facade(const vector_facade &);
		vector_facade(const vector_facade &, const allocator_type & alloc);
		vector_facade & operator =(const vector_facade &);

		vector_facade(vector_facade &&) noexcept = default;
		vector_facade(vector_facade &&, const allocator_type & alloc) noexcept( noexcept(is_nothrow_move_constructable()) );

		vector_facade & operator =(vector_facade &&) noexcept( noexcept(is_nothrow_move_assignable()) );

		inline friend void swap(vector_facade & s1, vector_facade & s2) noexcept
		{ using std::swap; swap(s1.m_storage, s2.m_storage); }
	};




	/************************************************************************/
	/*                    alloc methods block                               */
	/************************************************************************/
	template <class storage>
	inline auto vector_facade<storage>::check_newsize(size_type newsize) -> size_type
	{
		if (newsize >= max_size()) throw_xlen();
		return newsize;
	}

	template <class storage>
	auto vector_facade<storage>::check_newsize(size_type cursize, size_type increment) -> size_type
	{
		auto newsize = cursize + increment;
		// overflow or more than max_size
		// size_type is unsigned so overflow is well defined
		if (newsize < cursize)     throw_xlen();
		if (newsize >= max_size()) throw_xlen();

		return newsize;
	}

	template <class storage>
	inline auto vector_facade<storage>::allocate(allocator_type & alloc, size_type cap, const_pointer hint) -> std::pair<pointer, pointer>
	{
		auto ptr = allocator_traits::allocate(alloc, cap, hint);
		return {ptr, ptr + cap};
	}

	template <class storage>
	inline void vector_facade<storage>::deallocate(allocator_type & alloc, pointer ptr, size_type cap) noexcept
	{
		allocator_traits::deallocate(alloc, ptr, cap);
	}

	template <class storage>
	auto vector_facade<storage>::allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newcap, const_pointer hint) -> std::pair<pointer, pointer>
	{
		auto cap = (curcap / 2 <= newcap / 3) ? newcap : curcap + curcap / 2;
		//if (cap >= max_size()) cap = newcap;
		pointer newptr;

		if (cap < newcap or (newptr = allocator_traits::allocate(alloc, cap, hint)) == nullptr)
		{
			cap = newcap;
			newptr = allocator_traits::allocate(alloc, cap, hint);
		}

		return {newptr, newptr + cap};
	}

	template <class storage>
	inline auto vector_facade<storage>::checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type cursize, size_type increment, const_pointer hint) -> std::pair<pointer, pointer>
	{
		auto newsize = check_newsize(cursize, increment);
		return allocate_adjusted(alloc, curcap, newsize, hint);
	}

	template <class storage>
	inline auto vector_facade<storage>::checked_allocate_adjusted(allocator_type & alloc, size_type curcap, size_type newsize, const_pointer hint) -> std::pair<pointer, pointer>
	{
		check_newsize(newsize);
		return allocate_adjusted(alloc, curcap, newsize, hint);
	}

	/************************************************************************/
	/*             constructor/destructor stuff                             */
	/************************************************************************/
	template <class storage>
	vector_facade<storage>::~vector_facade() noexcept
	{
		pointer first, last, end;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		destroy(alloc, first, last);
		deallocate(alloc, first, end - first);
	}

	template <class storage>
	inline vector_facade<storage>::vector_facade(const allocator_type & alloc) noexcept
	    : m_storage(alloc)
	{

	}

	template <class storage>
	inline vector_facade<storage>::vector_facade(const vector_facade & other)
	    : vector_facade(other, allocator_traits::select_on_container_copy_construction(m_storage.get_allocator()))
	{

	}

	template <class storage>
	vector_facade<storage>::vector_facade(const vector_facade & other, const allocator_type & alloc)
	    : vector_facade(alloc)
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		decltype(auto) alloc_ = m_storage.get_allocator();
		std::tie(first, last, end) = other.m_storage.get_storage_pointers();
		std::tie(newfirst, newend) = allocate(alloc_, end - first, nullptr);

		try
		{
			newlast = uninitialized_copy(alloc_, first, last, newfirst);
		}
		catch (...)
		{
			deallocate(alloc_, newfirst, end - first);
			throw;
		}

		m_storage.set_storage_pointers({newfirst, newlast, newend});
	}

	template <class storage>
	vector_facade<storage> & vector_facade<storage>::operator =(const vector_facade<storage> & other)
	{
		if (this == &other) return *this;

		if constexpr(typename allocator_traits::is_always_equal() or not typename allocator_traits::propagate_on_container_copy_assignment())
			assign(other.begin(), other.end());
		else
		{
			decltype(auto) this_alloc = m_storage.get_allocator();
			decltype(auto) other_alloc = other.get_allocator();

			if (this_alloc != other_alloc)
			{
				pointer first, last, end;
				std::tie(first, last, end) = m_storage.get_storage_pointers();
				destroy(this_alloc, first, last);
				deallocate(this_alloc, first, last - first);
				first = last = end = nullptr;
				m_storage.set_storage_pointers({first, last, end});
				m_storage.set_allocator(other_alloc);
			}

			assign(other.begin(), other.end());
		}

		return *this;
	}

	template <class storage>
	vector_facade<storage>::vector_facade(vector_facade && other, const allocator_type & alloc) noexcept( noexcept(is_nothrow_move_constructable()) )
	    : vector_facade(alloc)
	{
		if constexpr(typename allocator_traits::is_always_equal())
			m_storage = std::move(other.m_storage);
		else
		{
			decltype(auto) this_alloc = m_storage.get_allocator();
			decltype(auto) other_alloc = other.get_allocator();

			if (this_alloc == other_alloc)
				m_storage = std::move(other.m_storage);
			else
			{
				pointer first, last, end;
				std::tie(first, last, end) = other.m_storage.get_storage_pointers();
				assign(std::make_move_iterator(first), std::make_move_iterator(first));

				destroy(other_alloc, first, last);
				other.deallocate(other_alloc, first, last - first);
				other.m_storage.set_storage_pointers({first, last, end});
			}
		}
	}

	template <class storage>
	vector_facade<storage> & vector_facade<storage>::operator =(vector_facade && other) noexcept( noexcept(is_nothrow_move_assignable()) )
	{
		if (this == &other) return *this;

		if constexpr(typename allocator_traits::is_always_equal() or not typename allocator_traits::propagate_on_container_move_assignment())
			m_storage = std::move(other.m_storage);
		else
		{
			decltype(auto) this_alloc = m_storage.get_allocator();
			decltype(auto) other_alloc = other.get_allocator();

			if (this_alloc == other_alloc)
				m_storage = std::move(other.m_storage);
			else
			{
				pointer first, last, end;
				std::tie(first, last, end) = other.m_storage.get_storage_pointers();
				assign(std::make_move_iterator(first), std::make_move_iterator(first));

				destroy(other_alloc, first, last);
				other.deallocate(other_alloc, first, last - first);
				other.m_storage.set_storage_pointers({first, last, end});
			}
		}

		return *this;
	}

	/************************************************************************/
	/*               emplace, push_back, pop_back methods block             */
	/************************************************************************/
	template <class storage>
	void vector_facade<storage>::pop_back()
	{
		pointer first, last, end;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		assert(first < last);
		destroy_at(alloc, last);
		last -= 1;

		m_storage.set_sotrage_pointers({first, last, end});
	}

	template <class storage>
	template <class ... Args>
	void vector_facade<storage>::emplace_back(Args && ... args)
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;

		// we have enough
		if (avail >= 1)
		{
			newfirst = first;
			newend = end;
			construct(alloc, last, std::forward<Args>(args)...);
			newlast = last + 1;
		}
		else
		{
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, 1, first);
			newlast = newfirst + size;

			try
			{
				construct(alloc, newfirst + size, std::forward<Args>(args)...);
				newlast += 1;

				uninitialized_move_if_noexcept_n(alloc, first, size, newfirst);
			}
			catch (...)
			{
				// destroy already constructed elements if there any
				if (newlast > newfirst + size) destroy_at(alloc, newfirst + size);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy_n(alloc, first, size);
			deallocate(alloc, first, capacity);
		}

		m_storage.set_storage_pointers({newfirst, newlast, newend});
	}

	template <class storage>
	template <class ... Args>
	auto vector_facade<storage>::emplace(const_iterator pos, Args && ... args) -> iterator
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type index = extract_pointer(pos) - first;
		pointer position = first + index;

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;

		if (avail >= 1)
		{
			// process in same storage
			auto_set_storage _(this, newfirst, newlast, newend);
			newfirst = first, newlast = last, newend = end;

			if (last == position)
				construct(alloc, newlast++, std::forward<Args>(args)...);
			else
			{
				construct(alloc, newlast++, std::move(*std::prev(last)));
				std::move_backward(position, std::prev(last), last);
				*position = value_type(std::forward<Args>(args)...);
			}
		}
		else
		{
			// allocate and process in new storage
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, 1, first);
			newlast = newfirst + size + 1;

			try
			{
				newlast = uninitialized_move_if_noexcept(alloc, first, position, newfirst);
				construct(alloc, newlast++, std::forward<Args>(args)...);
				newlast = uninitialized_move_if_noexcept(alloc, position, last, newlast);
			}
			catch (...)
			{
				destroy(alloc, newfirst, newlast);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy(alloc, first, last);
			deallocate(alloc, first, capacity);

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}

		return iterator(newfirst + index);
	}

	template <class storage>
	inline void vector_facade<storage>::push_back(const value_type & val)
	{
		return emplace_back(val);
	}

	template <class storage>
	inline void vector_facade<storage>::push_back(value_type && val)
	{
		return emplace_back(std::move(val));
	}


	/************************************************************************/
	/*          resize, reserve, shrink_to_fit methods block                */
	/************************************************************************/
	template <class storage>
	void vector_facade<storage>::reserve(size_type newcapacity)
	{
		if (newcapacity >= max_size()) throw_xlen();

		pointer first, last, end;
		pointer newfirst, newlast, newend;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;

		if (capacity >= newcapacity) return;

		std::tie(newfirst, newend) = allocate(alloc, newcapacity, first);
		newlast = newfirst + size;

		try
		{
			uninitialized_move_if_noexcept(alloc, first, last, newfirst);
		}
		catch (...)
		{
			deallocate(alloc, newfirst, newcapacity);
			throw;
		}

		destroy(alloc, first, last);
		deallocate(alloc, first, capacity);

		m_storage.set_storage_pointers({newfirst, newlast, newend});
	}

	template <class storage>
	void vector_facade<storage>::shrink_to_fit()
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;

		if (size == capacity) return;

		std::tie(newfirst, newend) = allocate(alloc, size, first);
		newlast = newend;

		try
		{
			uninitialized_move_if_noexcept(alloc, first, last, newfirst);
		}
		catch (...)
		{
			deallocate(alloc, newfirst, size);
			throw;
		}

		destroy(alloc, first, last);
		deallocate(alloc, first, capacity);

		m_storage.set_storage_pointers({newfirst, newlast, newend});
	}

	template <class storage>
	void vector_facade<storage>::resize(size_type n)
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;

		if (size > n)
		{
			destroy(alloc, first + n, last);
			m_storage.set_storage_pointers({first, last, end});
		}
		else if (size < n)
		{
			// we have enough
			if (avail >= n and use_relloc())
			{
				newfirst = first, newlast = last, newend = end;
				newlast = uninitialized_construct_n(alloc, default_constructor(), last, n);
			}
			else
			{
				std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, n, first);
				newlast = newfirst + size + n;

				pointer destroy_from = pointer();

				try
				{
					uninitialized_construct_n(alloc, default_constructor(), newfirst + size, n);
					destroy_from = newfirst + size;

					uninitialized_move_if_noexcept_n(alloc, first, size, newfirst);
				}
				catch (...)
				{
					// destroy already constructed elements if there any
					if (destroy_from) destroy_n(alloc, destroy_from, n);
					deallocate(alloc, newfirst, newend - newfirst);
					throw;
				}

				destroy_n(alloc, first, size);
				deallocate(alloc, first, capacity);
			}

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}
	}

	template <class storage>
	void vector_facade<storage>::resize(size_type n, const value_type & val)
	{
		using namespace ext::container;
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;

		if (size > n)
		{
			destroy(alloc, first + n, last);
			m_storage.set_storage_pointers({first, last, end});
		}
		else
		{
			// we have enough
			if (avail >= n)
			{
				newfirst = first, newlast = last, newend = end;
				newlast = uninitialized_construct_n(alloc, fill_constructor(val), last, n);
			}
			else
			{
				std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, n, first);
				newlast = newfirst + size + n;

				pointer destroy_from = pointer();

				try
				{
					uninitialized_construct_n(alloc, fill_constructor(val), newfirst + size, n);
					destroy_from = newfirst + size;

					uninitialized_move_if_noexcept_n(alloc, first, size, newfirst);
				}
				catch (...)
				{
					// destroy already constructed elements if there any
					if (destroy_from) destroy_n(alloc, destroy_from, n);
					deallocate(alloc, newfirst, newend - newfirst);
					throw;
				}

				destroy_n(alloc, first, size);
				deallocate(alloc, first, capacity);
			}

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}
	}


	/************************************************************************/
	/*                       insert methods block                           */
	/************************************************************************/
	template <class storage>
	auto vector_facade<storage>::insert(const_iterator pos, size_type insert_count, const value_type & val) -> iterator
	{
		if (insert_count == 0) return make_iterator(pos);

		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type index = extract_pointer(pos) - first;
		pointer position = first + index;

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;
		size_type after_count = last - position;

		if (avail >= insert_count and use_relloc())
		{
			auto_set_storage _(this, newfirst, newlast, newend);
			newfirst = first, newlast = last, newend = end;

			if (after_count > insert_count)
			{
				newlast = uninitialized_move_if_noexcept(alloc, last - insert_count, last, last);
				std::move_backward(position, last - insert_count, last);
				std::fill_n(position, insert_count, val);
			}
			else
			{
				newlast = uninitialized_construct_n(alloc, fill_constructor(val), last, insert_count - after_count);
				newlast = uninitialized_move_if_noexcept(alloc, position, last, newlast);
				std::fill_n(position, after_count, val);
			}
		}
		else
		{
			// allocate and process in new storage
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, insert_count, first);
			newlast = newfirst + size + insert_count;

			try
			{
				newlast = uninitialized_move_if_noexcept(alloc, first, position, newfirst);
				newlast = uninitialized_construct_n(alloc, fill_constructor(val), newlast, insert_count);
				newlast = uninitialized_move_if_noexcept(alloc, position, last, newlast);
			}
			catch (...)
			{
				destroy(alloc, newfirst, newlast);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy(alloc, first, last);
			deallocate(alloc, first, capacity);

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}

		return iterator(newfirst + index + insert_count);
	}

	template <class storage>
	template <class Iterator>
	auto vector_facade<storage>::do_range_insert(const_iterator pos, Iterator insert_first, Iterator insert_last, std::input_iterator_tag) -> iterator
	{
		self_type tmp(insert_first, insert_last);
		return do_range_insert(pos, tmp.begin(), tmp.end(), std::random_access_iterator_tag());
	}

	template <class storage>
	template <class Iterator>
	auto vector_facade<storage>::do_range_insert(const_iterator pos, Iterator insert_first, Iterator insert_last, std::forward_iterator_tag) -> iterator
	{
		if (insert_first == insert_last) return make_iterator(pos);

		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type index = extract_pointer(pos) - first;
		pointer position = first + index;

		size_type size = last - first;
		size_type capacity = end - first;
		size_type avail = capacity - size;
		size_type insert_count = std::distance(insert_first, insert_last);
		size_type after_count = last - position;

		if (avail >= insert_count and use_relloc())
		{
			// process in same storage
			auto_set_storage _(this, newfirst, newlast, newend);
			newfirst = first, newlast = last, newend = end;

			if (after_count > insert_count)
			{
				newlast = uninitialized_move_if_noexcept(alloc, last - insert_count, last, last);
				std::move_backward(position, last - insert_count, last);
				std::copy(insert_first, insert_last, position);
			}
			else
			{
				Iterator insert_mid = std::next(insert_first, after_count);
				newlast = uninitialized_copy(alloc, insert_mid, insert_last, last);
				newlast = uninitialized_move_if_noexcept(alloc, position, last, newlast);
				std::copy(insert_first, insert_mid, position);
			}
		}
		else
		{
			// allocate and process in new storage
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, size, insert_count, first);
			newlast = newfirst + size + insert_count;

			try
			{
				newlast = uninitialized_move_if_noexcept(alloc, first, position, newfirst);
				newlast = uninitialized_copy(alloc, insert_first, insert_last, newlast);
				newlast = uninitialized_move_if_noexcept(alloc, position, last, newlast);
			}
			catch (...)
			{
				destroy(alloc, newfirst, newlast);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy(alloc, first, last);
			deallocate(alloc, first, capacity);

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}

		return iterator(newfirst + index + insert_count);
	}


	/************************************************************************/
	/*                       assign methods block                           */
	/************************************************************************/
	template <class storage>
	void vector_facade<storage>::assign(size_type assign_count, const value_type & val)
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;

		if (capacity >= assign_count and use_relloc())
		{
			// process in same storage
			auto_set_storage _(this, newfirst, newlast, newend);
			newfirst = first, newlast = last, newend = end;

			if (assign_count >= size)
			{
				newlast = std::fill_n(newfirst, size, val);
				newlast = uninitialized_construct_n(alloc, fill_constructor(val), newlast, assign_count - size);
			}
			else
			{
				newlast = std::fill_n(newfirst, assign_count, val);
				destroy(alloc, newlast, last);
			}
		}
		else
		{
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, assign_count, first);
			newlast = newfirst + assign_count;

			try
			{
				newlast = uninitialized_construct_n(alloc, fill_constructor(val), newfirst, assign_count);
			}
			catch (...)
			{
				destroy(alloc, newfirst, newlast);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy(alloc, first, last);
			deallocate(alloc, first, capacity);

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}
	}

	template <class storage>
	template <class Iterator>
	void vector_facade<storage>::do_range_assign(Iterator assign_first, Iterator assign_last, std::input_iterator_tag)
	{
		pointer first, last, end, cur;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		for (cur = first; cur != last and assign_first != assign_last; (void)++cur, (void)++assign_first)
			*cur = *assign_first;

		if (assign_first == assign_last)
		{
			destroy(alloc, cur, last);
			m_storage.set_storage_pointers({first, cur, end});
		}
		else
		{
			do_range_insert(const_iterator(last), assign_first, assign_last, std::input_iterator_tag());
		}
	}

	template <class storage>
	template <class Iterator>
	void vector_facade<storage>::do_range_assign(Iterator assign_first, Iterator assign_last, std::forward_iterator_tag)
	{
		pointer first, last, end;
		pointer newfirst, newlast, newend;

		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type size = last - first;
		size_type capacity = end - first;
		size_type assign_count = std::distance(assign_first, assign_last);

		if (capacity >= assign_count and use_relloc())
		{
			// process in same storage
			auto_set_storage _(this, newfirst, newlast, newend);
			newfirst = first, newlast = last, newend = end;

			if (assign_count >= size)
			{
				newlast = std::copy_n(assign_first, size, newfirst);
				newlast = uninitialized_copy_n(alloc, std::next(assign_first, size), assign_count - size, newlast);
			}
			else
			{
				newlast = std::copy_n(assign_first, assign_count, newfirst);
				destroy(alloc, newlast, last);
			}
		}
		else
		{
			std::tie(newfirst, newend) = checked_allocate_adjusted(alloc, capacity, assign_count, first);
			newlast = newfirst + assign_count;

			try
			{
				newlast = uninitialized_copy(alloc, assign_first, assign_last, newfirst);
			}
			catch (...)
			{
				destroy(alloc, newfirst, newlast);
				deallocate(alloc, newfirst, newend - newfirst);
				throw;
			}

			destroy(alloc, first, last);
			deallocate(alloc, first, capacity);

			m_storage.set_storage_pointers({newfirst, newlast, newend});
		}
	}

	/************************************************************************/
	/*                       erase methods block                            */
	/************************************************************************/
	template <class storage>
	auto vector_facade<storage>::erase(const_iterator erase_first, const_iterator erase_last) -> iterator
	{
		pointer first, last, end;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		size_type erase_count = std::distance(erase_first, erase_last);

		last = std::move(extract_pointer(erase_last), last, extract_pointer(erase_first));
		destroy_n(alloc, last, erase_count);

		m_storage.set_storage_pointers({first, last, end});
		return make_iterator(erase_first);
	}

	template <class storage>
	auto vector_facade<storage>::erase(const_iterator where) -> iterator
	{
		pointer first, last, end;
		std::tie(first, last, end) = m_storage.get_storage_pointers();
		decltype(auto) alloc = m_storage.get_allocator();

		last = std::move(extract_pointer(where) + 1, last, extract_pointer(where));
		destroy_at(alloc, std::addressof(*last));

		m_storage.set_storage_pointers({first, last, end});
		return make_iterator(where);
	}

	/************************************************************************/
	/*                       at methods block                               */
	/************************************************************************/
	template <class storage>
	inline auto vector_facade<storage>::at(size_type pos) -> reference
	{
		pointer first, last;
		std::tie(first, last, std::ignore) = m_storage.get_storage_pointers();

		if (pos >= static_cast<size_type>(last - first))
			throw_xpos();

		return *(first + pos);
	}

	template <class storage>
	inline auto vector_facade<storage>::at(size_type pos) const -> const_reference
	{
		const_pointer first, last;
		std::tie(first, last, std::ignore) = m_storage.get_storage_pointers();

		if (pos >= static_cast<size_type>(last - first))
			throw_xpos();

		return *(first + pos);
	}

	/************************************************************************/
	/*                       comparators block                              */
	/************************************************************************/
	template <class storage>
	inline bool operator ==(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return v1.size() == v2.size() and std::equal(v1.begin(), v1.end(), v2.begin());
	}

	template <class storage>
	inline bool operator !=(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return not (v1 == v2);
	}

	template <class storage>
	inline bool operator <(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return std::lexicographical_compare(v1.begin(), v1.end(), v2.begin(), v2.end());
	}

	template <class storage>
	inline bool operator <=(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return not (v2 < v1);
	}

	template <class storage>
	inline bool operator >(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return v2 < v1;
	}

	template <class storage>
	inline bool operator >=(const vector_facade<storage> & v1, const vector_facade<storage> & v2)
	{
		return not (v1 < v2);
	}
}
