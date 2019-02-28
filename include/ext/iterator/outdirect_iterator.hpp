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
			using type = boost::iterator_adaptor<
				outdirect_iterator<Iterator, Value, Category, Reference, Difference>, // Iterator
				Iterator, // Base
				typename std::conditional<
					std::is_same<Value, boost::use_default>::value,
					std::add_pointer_t<typename boost::iterator_reference<Iterator>::type>, Value
				>::type,  // Value
				Category, // Traversal
				typename std::conditional<
					std::is_same<Reference, boost::use_default>::value,
					std::add_pointer_t<typename boost::iterator_reference<Iterator>::type>, Reference
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
	class outdirect_iterator :
		public detail::outdirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type
	{
		friend boost::iterator_core_access;

		using self_type = outdirect_iterator;

		using base_type = typename detail::outdirect_iterator_base<
			Iterator, Value, Category, Reference, Difference
		>::type;

	private:
		inline typename base_type::reference dereference() const
		{
			using ref_type = decltype(*this->base());
			static_assert(std::is_reference_v<ref_type>, "base iterator does not return reference, can't outdirect, dangling reference would be introduced");
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
