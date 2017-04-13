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
		template <class... Iterators>
		struct deduce_common_category;

		template <class Iterator, class... Iterators>
		struct deduce_common_category<Iterator, Iterators...>
		{
			typedef typename std::common_type<
				typename std::iterator_traits<Iterator>::iterator_category,
				typename deduce_common_category<Iterators...>::type
			>::type type;
		};

		template <class Iterator>
		struct deduce_common_category<Iterator>
		{
			typedef typename std::iterator_traits<Iterator>::iterator_category type;
		};

		template <class... Iterators>
		struct types
		{
			static_assert(sizeof...(Iterators) > 0, "empty iterator pack");

			typedef std::tuple<Iterators...> iterator_tuple;
			typedef std::tuple<typename std::iterator_traits<Iterators>::value_type...> value_tuple;
			typedef std::tuple<typename std::iterator_traits<Iterators>::reference...> ref_tuple;

			struct ref_type_helper : ref_tuple
			{
				//using ref_tuple::ref_tuple;

				template <std::size_t ... Indexes>
				static ref_tuple construct_helper(value_tuple & tuple, std::index_sequence<Indexes...>)
				{
					return ref_tuple(std::get<Indexes>(tuple)...);
				}

				template <class... Types>
				ref_type_helper(Types && ... args) : ref_tuple(std::forward<Types>(args)...) {}

				// conversions, needs more testing
				ref_type_helper(const value_tuple & op) : ref_tuple(op) {}
				ref_type_helper(value_tuple && op) : ref_tuple(construct_helper(op, std::index_sequence_for<Iterators...>{})) {}
				ref_type_helper(value_tuple & op) : ref_tuple(construct_helper(op, std::index_sequence_for<Iterators...>{})) {}
				//ref_type_helper(value_tuple & op) : ref_tuple(op) {}

				ref_type_helper & operator =(const value_tuple & op)
				{
					static_cast<const ref_tuple &>(*this) = op;
					return *this;
				}

				ref_type_helper & operator =(value_tuple && op)
				{
					static_cast<ref_tuple &>(*this) = std::move(op);
					return *this;
				}

				friend void swap(ref_type_helper & op1, ref_type_helper & op2)
				{
					std::swap(static_cast<ref_tuple &>(op1), static_cast<ref_tuple &>(op2));
				}

				friend void swap(ref_type_helper && op1, ref_type_helper && op2)
				{
					std::swap(static_cast<ref_tuple &>(op1), static_cast<ref_tuple &>(op2));
				}
			};

			typedef value_tuple value_type;
			typedef ref_type_helper reference_type;
			//typedef ref_tuple reference_type;
			typedef typename deduce_common_category<Iterators...>::type iterator_category;
			typedef typename std::iterator_traits<typename std::tuple_element<0, iterator_tuple>::type>::difference_type difference_type;
		};
	} //namespace zip_iterator_utils

	/// zip_iterator, supports reposition algorithms such as sort, partition
	/// at least n msvc 2013, gcc 4.8 ... 4.9+ , and probably others
	/// 
	/// it provides value_type as tuple<iterator::value_type...>,
	/// reference as type inherited from tuple of references, which provides swap for rvalues,
	/// allowing sort and other algorithms to work
	template <class... Iterators>
	class zip_iterator :
		public boost::iterator_facade<
			zip_iterator<Iterators...>,
			typename zip_iterator_utils::types<Iterators...>::value_type,
			typename zip_iterator_utils::types<Iterators...>::iterator_category,
			typename zip_iterator_utils::types<Iterators...>::reference_type,
			typename zip_iterator_utils::types<Iterators...>::difference_type
		>
	{
		friend boost::iterator_core_access;

		typedef boost::iterator_facade <
			zip_iterator<Iterators...>,
			typename zip_iterator_utils::types<Iterators...>::value_type,
			typename zip_iterator_utils::types<Iterators...>::iterator_category,
			typename zip_iterator_utils::types<Iterators...>::reference_type,
			typename zip_iterator_utils::types<Iterators...>::difference_type
		> base_type;

	public:
		typedef std::tuple<Iterators...> iterator_tuple_type;
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
		zip_iterator(const zip_iterator & op) = default;

		explicit zip_iterator(Iterators... args)
			: m_iterators(args...) {}

		explicit zip_iterator(const iterator_tuple_type & iterators)
			: m_iterators(iterators) {}

		const iterator_tuple_type & get_iterator_tuple() const { return m_iterators; }
	};

	template <class... Iterators>
	zip_iterator<Iterators...> make_zip_iterator(Iterators... iters)
	{
		return zip_iterator<Iterators...>{iters...};
	}


	/// same as std::get<0>(zip_iterator.get_iterator_tuple())
	template <std::size_t idx, class... Iterators>
	inline
	typename std::tuple_element<
		idx,
		typename zip_iterator<Iterators...>::iterator_tuple_type
	>::type const &
	get(const zip_iterator<Iterators...> & zip_iter)
	{
		return std::get<idx>(zip_iter.get_iterator_tuple());
	}
} // namespace ext
