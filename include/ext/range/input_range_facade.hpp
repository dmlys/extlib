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
	struct input_range_facade
	{
	public:
		class iterator : public boost::iterator_facade<iterator, Value, boost::single_pass_traversal_tag, Reference>
		{
			using base = boost::iterator_facade<iterator, Value, boost::single_pass_traversal_tag, Reference>;
			friend boost::iterator_core_access;
			friend input_range_facade;

			Derived * rng;
		private:
			typename base::reference dereference() const
			{
				return rng->front();
			}
			
			bool equal(const iterator & other) const
			{
				return rng == other.rng;
			}
			
			void increment()
			{
				rng->pop_front();
				if (rng->empty())
					rng = nullptr;
			}
			
			iterator(Derived * rng) : rng(rng)
			{
				if (this->rng->empty())
					this->rng = nullptr;
			}
		public:
			iterator() : rng(nullptr) {}
		};

		typedef typename iterator::value_type value_type;
		typedef typename iterator::reference reference;
		typedef typename iterator::pointer pointer;
		typedef typename iterator::difference_type difference_type;
		typedef std::size_t size_type;

		// for completeness and to boost range happy
		// (for example boost::make_iterator_range)
		typedef iterator const_iterator;
		typedef typename const_iterator::reference const_reference;
		typedef typename const_iterator::pointer const_pointer;

		iterator begin() { return static_cast<Derived*>(this); }
		iterator end() { return {}; }
	};
}
