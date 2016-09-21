#pragma once
#include <boost/iterator/iterator_adaptor.hpp>

namespace ext
{
	template <class Iterator, class Value, class Category, class Reference, class Difference>
	class outdirect_iterator;

	namespace detail
	{
		template <class Iterator, class Value, class Category, class Reference, class Difference>
		struct outdirect_iterator_base
		{
			typedef boost::iterator_adaptor<
				outdirect_iterator<Iterator, Value, Category, Reference, Difference>,
				Iterator,
				typename std::conditional<
					std::is_same<Value, boost::use_default>::value,
					typename std::add_pointer<typename boost::iterator_reference<Iterator>::type>::type, Value
				>::type,
				Category,
				typename std::conditional<
					std::is_same<Reference, boost::use_default>::value,
					typename std::add_pointer<typename boost::iterator_reference<Iterator>::type>::type, Value
				>::type,
				Difference
			> type;
		};
	}

	template <
		class Iterator,
		class Value      = boost::use_default,
		class Category   = boost::use_default,
		class Reference  = boost::use_default,
		class Difference = boost::use_default
	>
	class outdirect_iterator :
		public detail::outdirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type
	{
		friend boost::iterator_core_access;

		typedef outdirect_iterator self_type;

		typedef typename detail::outdirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type base_type;

	private:
		inline typename base_type::reference dereference() const
		{
			return &*this->base();
		}

	public:
		outdirect_iterator() = default;
		outdirect_iterator(Iterator iter) : base_type(iter) {}
	};


	template <class Iterator>
	inline outdirect_iterator<Iterator> make_outdirect_iterator(Iterator iter)
	{
		return outdirect_iterator<Iterator>(iter);
	}
}
