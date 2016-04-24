#pragma once
#include <memory>
#include <mutex>
#include <functional>

namespace ext
{
	/// represents a shared variable(by std::shared_ptr) which is created with first access
	/// and lives as long as there is valid shared_ptr to it
	/// instance created by user specified functor or by copy from value
	///
	/// examples:
	///   shared_by_need<int> si1 = 123;
	///   shared_by_need<std::string> sstr = std::string("123");
	///   shared_by_need<std::string> sstr2 = [] { return std::make_shared<std::string>("123"); };
	///
	/// Thread safety is managed by Mutex, which is by default is std::mutex,
	/// if you do not need thread safety - you can provide dummy_mutex
	template <class Type, class Mutex = std::mutex>
	class shared_by_need
	{
		shared_by_need(shared_by_need const &) = delete;
		shared_by_need & operator =(shared_by_need const &) = delete;

	private:
		using shared_ptr = std::shared_ptr<Type>;
		using weak_ptr = std::weak_ptr<Type>;

		weak_ptr m_weakPtr;
		Mutex m_mutex;
		std::function<shared_ptr()> m_creator;

	public:
		Mutex & get_mutex();

		/// returns creates and/or returns shared_ptr to instance
		shared_ptr accquire()
		{
			std::lock_guard<Mutex> lk(m_mutex);
			auto shared = m_weakPtr.lock();
			if (!shared)
				m_weakPtr = shared = m_creator();

			return shared;
		}

		/// from generic functor, which have to create shared_ptr
		shared_by_need(std::function<shared_ptr()> const & creator)
			: m_creator(creator) {}

		/// value copy constructor
		shared_by_need(Type const & val)
			: m_creator([val] { return std::make_shared<Type>(val); }) {}
	};
}