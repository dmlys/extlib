#pragma once
#include <boost/range.hpp>

namespace ext
{
	/// небольшое расширение boost::iterator_range
	/// реализует обертку над raw массивом.
	/// 
	/// В целом boost::iterator_range подходит полностью, за исключением отсутствия метода data(),
	/// который есть только у классов-контейнеров хранящих данные в непрерывной области памяти.
	/// как следствие алгоритм, требующий store contiguously, мог бы использовать метод data,
	/// но тогда он не сможет работать с boost::iterator_range<const Type *>
	/// 
	/// данный класс должен решить эту проблему
	/// так же он принимает не указатель а тип.
	template <class Type>
	class array_wrapper : public boost::iterator_range<Type *>
	{
		typedef boost::iterator_range<Type *> _base_type;

	public:
		      Type * data()       { return this->begin(); }
		const Type * data() const { return this->begin(); }

		//здесь можно применить inherited constructors, но их нету
		//пока дублируем
	public:
		array_wrapper() : _base_type() { }

		//! Constructor from a pair of iterators
		template<class Iterator>
		array_wrapper(Iterator begin, Iterator end)
			: _base_type(begin, end)
		{}

		//! Constructor from a Range
		template<class Range>
		array_wrapper(Range && r)
			: _base_type(std::forward<Range>(r))
		{}
	};

	template <class Type>
	class array_wrapper<const Type> : public boost::iterator_range<const Type *>
	{	
		typedef boost::iterator_range<const Type *> _base_type;
	
	public:
		const Type * data() const { return this->begin(); }

		//здесь можно применить inherited constructors, но их нету
		//пока дублируем
	public:
		array_wrapper() : _base_type() { }

		//! Constructor from a pair of iterators
		template<class Iterator>
		array_wrapper(Iterator begin, Iterator end)
			: _base_type(begin, end)
		{}

		//! Constructor from a Range
		template<class Range>
		array_wrapper(Range && r)
			: _base_type(std::forward<Range>(r))
		{}
	};

	template <class Type>
	array_wrapper<Type> make_array_wrapper(Type * beg, Type * end) { return array_wrapper<Type>(beg, end); }

	template <class Type>
	array_wrapper<Type> make_array_wrapper(Type * beg, std::size_t sz) { return array_wrapper<Type>(beg, beg + sz); }

	template <class Type, std::size_t N>
	array_wrapper<Type> make_array_wrapper(Type (&buffer)[N]) { return array_wrapper<Type>(buffer, buffer + N); }
}