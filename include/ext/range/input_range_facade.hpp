#pragma once
#include <boost/iterator/iterator_facade.hpp>

/// NOTE: не плохо бы иметь аналог iterator_facade,
/// но для этого нужно определить категории ranges и их семантику

/// данный класс был вдохновлен следующей статьей и комментариями к ней:
/// http://ericniebler.com/2013/11/07/input-iterators-vs-input-ranges/

namespace ext
{
	/// input_range_facade аналог boost::iterator_facade но только для intput range(в будущем нужно будет попробовать обобщить)
	/// интерфейс наследуемого класса следующий:
	/// - bool empty() - true if range is empty
	/// - reference front() - returns reference to last read value
	/// - void pop_front() - drops last value, and reads new one
	///
	/// наследуемый класс должен сделать pop_front в конструкторе
	///
	/// usage pattern:
	///   irange rng;
	///   while(!rng.empty())
	///   {
	///      val = rng.front();
	///      ...
	///      rng.pop_front();
	///   }
	///
	/// @Param Derived - derived class which implements interface
	/// @Param Value - value type of range, int, string, anything
	/// @Param Reference - reference to value
	template <class Derived, class Value, class Reference = Value &>
	class input_range_facade;

	template <class Derived, class Value, class Reference>
	class input_range_facade_iterator;

	template <class Derived, class Value, class Reference>
	class input_range_facade_const_iterator;





	template <class Derived, class Value, class Reference>
	class input_range_facade
	{
		using self_type = input_range_facade;

	public:
		using iterator = input_range_facade_iterator<Derived, Value, Reference>;
		using const_iterator = input_range_facade_const_iterator<Derived, Value, Reference>;

		using value_type      = typename iterator::value_type;
		
		using reference       = typename iterator::reference;
		using pointer         = typename iterator::pointer;

		using const_reference = typename const_iterator::reference;
		using const_pointer   = typename const_iterator::pointer;

		using difference_type = typename iterator::difference_type;
		using size_type = std::size_t;

		iterator begin() noexcept { return static_cast<Derived*>(this); }
		iterator end()   noexcept { return {}; }

		const_iterator begin() const noexcept { return static_cast<const Derived*>(this); }
		const_iterator end()   const noexcept { return {}; }

		const_iterator cbegin() const noexcept { return static_cast<const Derived*>(this); }
		const_iterator cend()   const noexcept { return {}; }
	};



	template <class Derived, class Value, class Reference>
	class input_range_facade_const_iterator :
		public boost::iterator_facade<
			input_range_facade_const_iterator<Derived, Value, Reference>,
			Value,
			boost::single_pass_traversal_tag,
			Reference
		>
	{
		using self_type = input_range_facade_const_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			Value,
			boost::single_pass_traversal_tag,
			Reference
		>;

		friend boost::iterator_core_access;
		friend input_range_facade<Derived, Value, Reference>;
		friend input_range_facade_iterator<Derived, Value, Reference>;

	public:
		using typename base_type::reference;

	private:
		const Derived * rng;

	private:
		reference dereference() const { return rng->front(); }
		bool equal(const self_type & other) const noexcept { return rng == other.rng; }

		void increment()
		{
			rng->pop_front();
			if (rng->empty())
				rng = nullptr;
		}

		input_range_facade_const_iterator(const Derived * rng) : rng(rng)
		{
			if (this->rng->empty())
				this->rng = nullptr;
		}

	public:
		input_range_facade_const_iterator() : rng(nullptr) {}
		input_range_facade_const_iterator(input_range_facade_iterator<Derived, Value, Reference> it)
			: rng(it.rng) {}
	};



	template <class Derived, class Value, class Reference>
	class input_range_facade_iterator :
		public boost::iterator_facade<
			input_range_facade_iterator<Derived, Value, Reference>,
			Value,
			boost::single_pass_traversal_tag,
			Reference
		>
	{
		using self_type = input_range_facade_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			Value,
			boost::single_pass_traversal_tag,
			Reference
		>;

		friend boost::iterator_core_access;
		friend input_range_facade<Derived, Value, Reference>;
		friend input_range_facade_const_iterator<Derived, Value, Reference>;

	public:
		using typename base_type::reference;

	private:
		Derived * rng;

	private:
		reference dereference() const { return rng->front(); }
		bool equal(const self_type & other) const noexcept { return rng == other.rng; }

		void increment()
		{
			rng->pop_front();
			if (rng->empty())
				rng = nullptr;
		}

		input_range_facade_iterator(Derived * rng) : rng(rng)
		{
			if (this->rng->empty())
				this->rng = nullptr;
		}

	public:
		input_range_facade_iterator() : rng(nullptr) {}
		// constructing iterator from const_iterator is dissalowed
		input_range_facade_iterator(input_range_facade_const_iterator<Derived, Value, Reference> it) = delete;
	};
}
