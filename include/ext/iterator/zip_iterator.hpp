#pragma once
#include <type_traits>
#include <iterator>
#include <tuple>
#include <ext/utility.hpp>

#include <boost/iterator/iterator_facade.hpp>

/// WARN: experimental,
/// can have some compile problems on some platforms
namespace ext
{
	namespace zip_iterator_utils
	{
		// zip_iterator should work with std, boost and and other custom iterators.
		// There is:
		// * std category set: std::random_access_iterator_tag, ...
		// * boost categories: boost::random_access_traversal_tag, ...
		// 
		// zip_iterator must deduce_common_traversal for it's iterators,
		// some can be std::*_iterator_tag, others boost::*_traversal_tag.
		// 
		// Implementation uses boost::iterator_facade<..., value_type, traversal, reference, ...>,
		// which have following behavior: if traversal is from std::*_iterator_tag - use as is.
		// Otherwise it's CategoryOrTraversal structure which is inherited both
		// from std::*_iterator_tag, and boost::*_traversal_tag.
		// 
		// But if reference is not actually a reference,
		// than std category would be no higher than std::input_iterator_category,
		// even if traversal is boost::random_access_traversal_tag.
		// 
		// That ok for iterators such as transform_iterator,
		// but not for zip_iterator where reference = tuple<type &...> -
		// std library will not work with those iterators.
		// 
		// So we deduce common category as boost traversal and than convert it into std iterator category
		// 

		template <class traversal>
		struct boost_traversal_to_std_category
		{
			using type = traversal;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::random_access_traversal_tag>
		{
			using type = std::random_access_iterator_tag;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::bidirectional_traversal_tag>
		{
			using type = std::bidirectional_iterator_tag;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::forward_traversal_tag>
		{
			using type = std::forward_iterator_tag;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::single_pass_traversal_tag>
		{
			using type = std::input_iterator_tag;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::incrementable_traversal_tag>
		{
			using type = std::output_iterator_tag;
		};

		template <>
		struct boost_traversal_to_std_category<boost::iterators::no_traversal_tag>
		{
			// no type
		};


		template <class Iterator>
		using get_category_t = typename boost_traversal_to_std_category<
			typename boost::iterators::pure_iterator_traversal<Iterator>::type
		>::type;



		template <class... Iterators>
		struct deduce_common_category
		{
			using type = std::common_type_t<
				get_category_t<Iterators>...
			>;
		};

		template <class... Iterators>
		using deduce_common_category_t = typename deduce_common_category<Iterators...>::type;


		template <class ... Types>
		class ref_type_helper : public std::tuple<Types...>
		{
		public:
			using self_type = ref_type_helper;
			using base_type = std::tuple<Types...>;
			using value_tuple = std::tuple<std::remove_reference_t<Types>...>;

			using base_type::base_type;

			//template <std::size_t ... Indexes>
			//static ref_tuple construct_helper(value_tuple & tuple, std::index_sequence<Indexes...>)
			//{
			//	return ref_tuple(std::get<Indexes>(tuple)...);
			//}

			//template <class... Types>
			//ref_type_helper(Types && ... args) : ref_tuple(std::forward<Types>(args)...) {}

			//// conversions, needs more testing
			//ref_type_helper(const value_tuple & op) : ref_tuple(op) {}
			//ref_type_helper(value_tuple && op) : ref_tuple(construct_helper(op, std::index_sequence_for<Iterators...>{})) {}
			//ref_type_helper(value_tuple & op) : ref_tuple(construct_helper(op, std::index_sequence_for<Iterators...>{})) {}
			////ref_type_helper(value_tuple & op) : ref_tuple(op) {}

			self_type & operator =(const value_tuple & op)
			{
				static_cast<const base_type &>(*this) = op;
				return *this;
			}

			self_type & operator =(value_tuple && op)
			{
				static_cast<base_type &>(*this) = std::move(op);
				return *this;
			}

			friend void swap(self_type & op1, self_type & op2)
			{
				std::swap(static_cast<base_type &>(op1), static_cast<base_type &>(op2));
			}

			friend void swap(ref_type_helper && op1, ref_type_helper && op2)
			{
				std::swap(static_cast<base_type &>(op1), static_cast<base_type &>(op2));
			}
		};


		template <class... Iterators>
		struct types
		{
			static_assert(sizeof...(Iterators) > 0, "empty iterator pack");

			using iterator_tuple = std::tuple<Iterators...>;
			using value_tuple = std::tuple<typename std::iterator_traits<Iterators>::value_type...>;
			using ref_tuple = std::tuple<typename std::iterator_traits<Iterators>::reference...>;
			using ref_type_helper = ext::zip_iterator_utils::ref_type_helper<
				typename std::iterator_traits<Iterators>::reference...
			>;

			using value_type = value_tuple;
			using reference_type = ref_type_helper;
			//using reference_type = ref_tuple;

			using iterator_traversal = deduce_common_category_t<Iterators...>;

			//using difference_type = typename std::iterator_traits<typename std::tuple_element<0, iterator_tuple>::type>::difference_type;
			using difference_type = std::common_type_t<
				typename std::iterator_traits<Iterators>::difference_type...
			>;
		};
	} //namespace zip_iterator_utils

	/// zip_iterator, supports reposition algorithms such as sort, partition
	/// at least on msvc 2017, gcc 8 , and probably others
	/// 
	/// it provides value_type as tuple<iterator::value_type...>,
	/// reference as type inherited from tuple of references, which provides swap for rvalues,
	/// allowing sort and other algorithms to work
	template <class... Iterators>
	class zip_iterator :
		public boost::iterator_facade<
			zip_iterator<Iterators...>,
			typename zip_iterator_utils::types<Iterators...>::value_type,
			typename zip_iterator_utils::types<Iterators...>::iterator_traversal,
			typename zip_iterator_utils::types<Iterators...>::reference_type,
			typename zip_iterator_utils::types<Iterators...>::difference_type
		>
	{
		friend boost::iterator_core_access;

		typedef boost::iterator_facade <
			zip_iterator<Iterators...>,
			typename zip_iterator_utils::types<Iterators...>::value_type,
			typename zip_iterator_utils::types<Iterators...>::iterator_traversal,
			typename zip_iterator_utils::types<Iterators...>::reference_type,
			typename zip_iterator_utils::types<Iterators...>::difference_type
		> base_type;

	public:
		using iterator_tuple_type = std::tuple<Iterators...>;
		using typename base_type::reference;
		using typename base_type::difference_type;

	private:
		iterator_tuple_type m_iterators;

	private:
		template <std::size_t... Indexes>
		void increment_iterator_tuple(std::index_sequence<Indexes...>)
		{
			ext::aux_pass(++std::get<Indexes>(m_iterators)...);
		}

		template <std::size_t... Indexes>
		void decrement_iterator_tuple(std::index_sequence<Indexes...>)
		{
			ext::aux_pass(--std::get<Indexes>(m_iterators)...);
		}

		template <std::size_t... Indexes>
		reference dereference_iterator_tuple(std::index_sequence<Indexes...>) const
		{
			return reference {*std::get<Indexes>(m_iterators)...};
		}

		template <std::size_t... Indexes>
		void advance_iterator_tuple(difference_type n, std::index_sequence<Indexes...>)
		{
			ext::aux_pass(std::get<Indexes>(m_iterators) += n ...);
		}

		template <std::size_t... Indexes>
		difference_type distance_iterator_tuple(const zip_iterator & other) const
		{
			return std::get<0>(other.m_iterators) - std::get<0>(m_iterators);
		}

	private:
		void increment()
		{
			increment_iterator_tuple(std::index_sequence_for<Iterators...>());
		}

		void decrement()
		{
			decrement_iterator_tuple(std::index_sequence_for<Iterators...>());
		}

		bool equal(const zip_iterator & op) const
		{
			return m_iterators == op.m_iterators;
		}

		reference dereference() const
		{
			return dereference_iterator_tuple(std::index_sequence_for<Iterators...>());
		}

		void advance(difference_type n)
		{
			advance_iterator_tuple(n, std::index_sequence_for<Iterators...>());
		}

		difference_type distance_to(const zip_iterator & op) const
		{
			return distance_iterator_tuple(op);
		}

	public:
		zip_iterator() = default;

		explicit zip_iterator(Iterators... args)
			: m_iterators(std::move(args)...) {}

		explicit zip_iterator(const iterator_tuple_type & iterators)
			: m_iterators(iterators) {}

		const iterator_tuple_type & get_iterator_tuple() const { return m_iterators; }
	};

	template <class... Iterators>
	zip_iterator<Iterators...> make_zip_iterator(Iterators... iters)
	{
		return zip_iterator<Iterators...>{std::move(iters)...};
	}


	/// same as std::get<0>(zip_iterator.get_iterator_tuple())
	template <std::size_t idx, class... Iterators>
	inline auto get(const zip_iterator<Iterators...> & zip_iter)
		-> const std::tuple_element_t<idx, typename zip_iterator<Iterators...>::iterator_tuple_type> &
	{
		return std::get<idx>(zip_iter.get_iterator_tuple());
	}
} // namespace ext


namespace std
{
	template <std::size_t Idx, class ... Types>
	struct tuple_element<Idx, ext::zip_iterator_utils::ref_type_helper<Types...>>
		: tuple_element<Idx, std::tuple<Types...>> {};

	template <class ... Types>
	struct tuple_size<ext::zip_iterator_utils::ref_type_helper<Types...>>
		: tuple_size<std::tuple<Types...>> {};
}
