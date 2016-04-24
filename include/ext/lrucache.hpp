#pragma once
#include <vector>
#include <functional>
#include <ext/functors/get_func.hpp> //for ext::first_el для batch_lru_cache

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/call_traits.hpp>
#include <boost/swap.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>

namespace ext
{
	/// lru кеш
	/// реализация подсмотрена здесь http://timday.bitbucket.org/lru.html,
	/// но есть небольшие изменения
	/// кеш разделен на 3 случая: manual_lru_cache, где пользователь сам заполняет кеш
	///                          lru_cache оригинальный кеш, получающий по одной записи за раз
	///                          batch_lru_cache кеш получающий пачку записей за раз
	///
	/// для получения данных используется не std::function, а функтор
	/// специальные функторы function_acquire/batch_function_acquire позволяют не писать новый функтор,
	/// а просто передать некое выражение в std::function<bool (Key, Val)>/std::function<bool (Key, vector<pair<Key, Val>> & )>

	/// кеш с ручной подгрузкой данных
	template <
		class Key,
		class Value,
		class Hash = boost::hash<Key>,
		class KeyEqual = std::equal_to<>
	>
	class manual_lru_cache
	{
	public:
		typedef typename boost::call_traits<Key>::param_type key_param;
		typedef typename boost::call_traits<Value>::param_type value_param;
		
		typedef Key key_type;
		typedef Value mapped_type;
		typedef Hash hasher;
		typedef KeyEqual key_equal;

	private:
		struct entry
		{
			key_type key;
			mapped_type value;

			entry(key_type && key, mapped_type && value)
				: key(std::move(key)), value(std::move(value)) {}
		};

		typedef boost::multi_index_container <
			entry,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::member<entry, key_type, &entry::key>,
					hasher,
					key_equal
				>,
				boost::multi_index::sequenced<>
			>
		> cache_conatiner;

		static const std::size_t ByCode = 0;
		static const std::size_t ByPos = 1;

		typedef typename cache_conatiner::template nth_index<ByCode>::type  code_view;
		typedef typename cache_conatiner::template nth_index<ByPos>::type   pos_view;

	private:
		cache_conatiner m_cache;
		std::size_t m_cache_maxsize;

		void touch(typename code_view::iterator it)
		{
			auto & pv = m_cache.template get<ByPos>();
			auto posIt = m_cache.template project<ByPos>(it);
			// перемещаем элемент по it в конец списка
			pv.relocate(pv.end(), posIt);
		}

	public:
		/// скидывает наиболее давно используемый элемент
		void drop_last()
		{
			auto & pv = m_cache.template get<ByPos>();
			pv.pop_front();
		}

		mapped_type & insert(key_type key, mapped_type data)
		{
			bool inserted;
			typename code_view::iterator pos;
			std::tie(pos, inserted) = m_cache.emplace(std::move(key), std::move(data));
			
			if (!inserted) {
				// const_cast is safe because our index is only by key
				auto & val = const_cast<mapped_type &>(pos->value);
				boost::swap(val, data);
				touch(pos);
				return val;
			}
			else {
				BOOST_ASSERT_MSG(m_cache_maxsize > 0, "lru_cache can't work with CacheMaxSize == 0");
				if (m_cache_maxsize < m_cache.size())
					drop_last();
				
				// const_cast is safe because our index is only by key
				return const_cast<mapped_type &>(pos->value);
			}
		}

		/// получает данные по ключу, если таких данных нет, то throws std::out_of_range
		mapped_type & at(key_param key)
		{
			auto * val = find_ptr(key);
			if (val)
				return *val;
			else
				throw std::out_of_range("lru_cache out of range");
		}
		/// получает данные по ключу, если таких данных нет, то returns nullptr
		mapped_type * find_ptr(key_param key)
		{
			auto it = m_cache.find(key);
			if (it == m_cache.end())
				return nullptr;
			else
			{
				touch(it);
				// const_cast is safe because our index is only by key
				return &const_cast<mapped_type &>(it->value);
			}
		}

		/// сбрасывает кеш
		void clear()                 { m_cache.clear(); }
		std::size_t size() const     { return m_cache.size(); }
		std::size_t maxsize() const  { return m_cache_maxsize; }

		/// скидывает элменты так что бы размер кеша не превышал size
		void drop_to(std::size_t size)
		{
			auto & pv = m_cache.template get<ByPos>();
			for (auto cursz = m_cache.size(); cursz > size; --cursz)
				pv.pop_front();
		}

		void set_maxsize(std::size_t size)
		{
			if (size == 0)
				throw std::invalid_argument("lru_cache: CacheMaxSize == 0 is invalid");
			
			drop_to(size);
			m_cache_maxsize = size;
		}

		explicit manual_lru_cache(std::size_t size)
			: m_cache_maxsize(size) {}

		manual_lru_cache(const manual_lru_cache &) = delete;
		manual_lru_cache & operator =(const manual_lru_cache &) = delete;

		manual_lru_cache(manual_lru_cache && r) BOOST_NOEXCEPT
			: m_cache(std::move(r.m_cache)),
			  m_cache_maxsize(r.m_cache_maxsize) {}

		manual_lru_cache & operator =(manual_lru_cache && r) BOOST_NOEXCEPT
		{
			manual_lru_cache tmp = std::move(r);
			swap(tmp);
		}
		
		void swap(manual_lru_cache & other) BOOST_NOEXCEPT
		{
			boost::swap(m_cache, other.m_cache);
			boost::swap(m_cache_maxsize, other.m_cache_maxsize);
		}
	};

	template <class Key, class Value, class Hash, class KeyEqual>
	inline void swap(manual_lru_cache<Key, Value, Hash, KeyEqual> & c1,
	                 manual_lru_cache<Key, Value, Hash, KeyEqual> & c2) BOOST_NOEXCEPT
	{
		c1.swap(c2);
	}

	/// lru_cache умеющий получать записи автоматически с помощью функтора Acquire
	/// выражение:
	/// Value v = Acquire(key) должно быть валидным
	template <
		class Key,
		class Value,
		class Hash = boost::hash<Key>,
		class KeyEqual = std::equal_to<>,
		class Acquire = std::function<Value(const Key &)>
	>
	class lru_cache : private manual_lru_cache<Key, Value, Hash, KeyEqual>
	{
		typedef manual_lru_cache<Key, Value, Hash, KeyEqual> base_type;
		
	public:
		using typename base_type::key_type;
		using typename base_type::mapped_type;
		using typename base_type::hasher;
		using typename base_type::key_equal;
		using typename base_type::key_param;

	private:
		Acquire m_Acquire;

		mapped_type * acquire(key_param key)
		{
			Value val = m_Acquire(key);
			return &base_type::insert(std::move(key), std::move(val));
		}

	public:
		using base_type::clear;
		using base_type::size;
		using base_type::maxsize;
		using base_type::drop_last;
		using base_type::drop_to;
		using base_type::set_maxsize;
	
		/// получает данные по ключу
		mapped_type & at(key_param key)
		{
			auto * val = base_type::find_ptr(key);
			if (!val) val = acquire(key);
			return *val;
		}

		explicit lru_cache(std::size_t size, Acquire ac)
			: base_type(size), m_Acquire(std::move(ac)) {}

		lru_cache(const lru_cache &) = delete;
		lru_cache & operator =(const lru_cache &) = delete;

		lru_cache(lru_cache && r) BOOST_NOEXCEPT
			: base_type(std::move(r)), m_Acquire(std::move(r.m_Acquire)) {}

		lru_cache & operator =(lru_cache && r) BOOST_NOEXCEPT
		{
			lru_cache tmp = std::move(r);
			swap(tmp);
		}

		void swap(lru_cache & other) BOOST_NOEXCEPT
		{
			base_type::swap(other);
			boost::swap(m_Acquire, other.m_Acquire);
		}
	};

	template <class Key, class Value, class Hash, class KeyEqual, class Acquire>
	inline void swap(lru_cache<Key, Value, Hash, KeyEqual, Acquire> & c1,
	                 lru_cache<Key, Value, Hash, KeyEqual, Acquire> & c2) BOOST_NOEXCEPT
	{
		c1.swap(c2);
	}

	/// lru_cache умеющий получать записи автоматически с помощью функтора Acquire
	///
	/// выражение:
	/// auto data = Acquire(key, size(), maxsize());
	/// должно быть валидным
	/// возвращаемое значение - stl compatible range of tuple like objects,
	/// where get<0>(val) - returns key, get<1>(val) - returns value
	template <
		class Key,
		class Value,
		class Hash,
		class KeyEqual,
		class Acquire
	>
	class batch_lru_cache : private manual_lru_cache<Key, Value>
	{
		typedef manual_lru_cache<Key, Value, Hash, KeyEqual> base_type;

	public:
		using typename base_type::key_type;
		using typename base_type::mapped_type;
		using typename base_type::hasher;
		using typename base_type::key_equal;
		using typename base_type::key_param;

	private:
		Acquire m_Acquire;

		template <class DataContainer>
		void merge(DataContainer && data)
		{
			first_el first;
			second_el second;
			for (auto && val : data)
			{
				typedef typename std::decay<decltype(first(val))>::type KeyType;
				typedef typename std::decay<decltype(second(val))>::type ValueType;

				static_assert(std::is_convertible<KeyType, key_type>::value, "Key mismatch");
				static_assert(std::is_convertible<ValueType, mapped_type>::value, "Value mismatch");

				base_type::insert(std::move(first(val)), std::move(second(val)));
			}
		}

		Value * acquire(key_param key)
		{
			auto data = m_Acquire(key, size(), maxsize());
			merge(std::move(data));
			return base_type::find_ptr(key);
		}

	public:
		using base_type::clear;
		using base_type::size;
		using base_type::maxsize;
		using base_type::drop_last;
		using base_type::drop_to;
		using base_type::set_maxsize;

		/// получает данные по ключу
		mapped_type & at(key_param key)
		{
			auto * val = find_ptr(key);
			if (!val) val = acquire(key);
			return *val;
		}

		explicit batch_lru_cache(std::size_t maxSize, Acquire ac)
			: base_type(maxSize), m_Acquire(std::move(ac)) {}
			
		batch_lru_cache(const batch_lru_cache &) = delete;
		batch_lru_cache & operator =(const batch_lru_cache &) = delete;

		batch_lru_cache(batch_lru_cache && r) BOOST_NOEXCEPT
			: base_type(std::move(r)), m_Acquire(std::move(r.m_Acquire)) {}

		batch_lru_cache & operator =(batch_lru_cache && r) BOOST_NOEXCEPT
		{
			batch_lru_cache tmp = std::move(r);
			swap(tmp);
		}

		void swap(batch_lru_cache & other) BOOST_NOEXCEPT
		{
			base_type::swap(other);
			boost::swap(m_Acquire, other.m_Acquire);
		}
	};

	template <class Key, class Value, class Hash, class KeyEqual, class Acquire>
	inline void swap(batch_lru_cache<Key, Value, Hash, KeyEqual, Acquire> & c1,
	                 batch_lru_cache<Key, Value, Hash, KeyEqual, Acquire> & c2) BOOST_NOEXCEPT
	{
		c1.swap(c2);
	}
}