#pragma once
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/indirect_iterator.hpp>

namespace ext
{
	template <class Iterator, class Value, class Category, class Reference, class Difference>
	class indirect_iterator;

	namespace detail
	{
		template <class Iterator, class Value, class Category, class Reference, class Difference>
		struct indirect_iterator_base
		{
			using deduced_reference = decltype(*std::declval<typename boost::iterator_reference<Iterator>::type>());
			using dedeuced_value_type = std::remove_reference_t<deduced_reference>;

			using type = boost::iterator_adaptor<
				indirect_iterator<Iterator, Value, Category, Reference, Difference>,
				Iterator,  // Base
				typename std::conditional<
					std::is_same<Value, boost::use_default>::value,
					dedeuced_value_type, Value
				>::type,   // Value
				Category,  // Traversal
				typename std::conditional<
					std::is_same<Reference, boost::use_default>::value,
					deduced_reference, Reference
				>::type,   // Reference
				Difference // Difference
			>;
		};
	}

	template <
		class Iterator,
		class Value      = boost::use_default,
		class Category   = boost::use_default,
		class Reference  = boost::use_default,
		class Difference = boost::use_default
	>
	class indirect_iterator :
		public detail::indirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type
	{
		friend boost::iterator_core_access;

		using self_type = indirect_iterator ;

		using base_type = typename detail::indirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type ;

	private:
		inline typename base_type::reference dereference() const
		{
			return **this->base();
		}

	public:
		indirect_iterator() = default;
		indirect_iterator(Iterator iter) : base_type(iter) {}
	};


	template <class Iterator>
	inline indirect_iterator<Iterator> make_indirect_iterator(Iterator iter)
	{
		return indirect_iterator<Iterator>(iter);
	}

}
