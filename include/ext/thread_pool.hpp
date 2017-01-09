#pragma once
#include <memory>
#include <vector>

#include <mutex>
#include <condition_variable>
#include <thread>

#include <boost/intrusive/list.hpp>
#include <ext/intrusive_ptr.hpp>
#include <ext/future.hpp>

namespace ext
{
	/// thread_pool executioner implements simple thread pool.
	/// Task can be submitted via submit method.
	/// For every task result of execution can be retrieved via associated future.
	/// Number of running threads can be controlled via set_nworkers/get_nworkers methods.
	/// By default thread_pool constructed with 0 workers.
	/// 
	/// All methods are thread-safe
	class thread_pool
	{
	private:
		/// base interface for submitted tasks, 
		/// tasks are hold in intrusive linked list.
		class task_base : 
			public boost::intrusive::list_base_hook<
				boost::intrusive::link_mode<boost::intrusive::link_mode_type::normal_link>
			>
		{
		public:
			virtual ~task_base() = default;

			// note: addref, release are present in ext::packaged_task_impl, that can issue a conflict,
			// if signature same, but return value different - it's a error.
			// just name them differently, it's internal private class.
			virtual void task_addref()  noexcept = 0;
			virtual void task_release() noexcept = 0;
			virtual void task_abandone() noexcept = 0;
			virtual void task_execute() noexcept = 0;

		public:
			friend inline void intrusive_ptr_add_ref(task_base * ptr) noexcept { if (ptr) ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_base * ptr) noexcept { if (ptr) ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_base * ptr) noexcept {}
		};

		/// implementation of task_base templated by Functor
		template <class Functor, class ResultType>
		class task_impl :
			public task_base,
			public ext::packaged_task_impl<Functor, ResultType()>
		{
			typedef ext::packaged_task_impl<Functor, ResultType()> base_type;

		public:
			void task_addref()  noexcept override { base_type::addref(); }
			void task_release() noexcept override { base_type::release(); }
			void task_abandone() noexcept override { base_type::release_promise(); }
			void task_execute() noexcept override { base_type::execute(); }

		public:
			// inherit constructors
			using base_type::base_type;

		public:
			friend inline void intrusive_ptr_add_ref(task_impl * ptr) noexcept { if (ptr) ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_impl * ptr) noexcept { if (ptr) ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_impl * ptr) noexcept {}
		};
		
		/// thread worker object, also a future. When thread is finished - future becomes fulfilled
		class worker : public ext::shared_state_unexceptional<void>
		{
			typedef ext::shared_state_unexceptional<void> base_type;

		private:
			thread_pool * m_parent;
			std::thread m_thread;
			std::atomic_bool m_stop_request = ATOMIC_VAR_INIT(false);

		private:
			static void thread_func(ext::intrusive_ptr<worker> self);
			
		public:
			bool stop_request() noexcept;

		public:
			worker(thread_pool * parent);
		};

	private:
		typedef ext::intrusive_ptr<worker> worker_ptr;

		typedef boost::intrusive::list<
			task_base,
			boost::intrusive::constant_time_size<false>
		> task_list_type;

	private:
		/// linked list of task 
		task_list_type m_tasks;

		/// vector of worker objects, it also holds workers that are stopping.
		/// vector is always partitioned by working/stopping: 
		///   [0, m_pending)    - working threads
		///   [m_pending, last) - stopping threads
		std::vector<worker_ptr> m_workers;
		std::size_t m_pending = 0;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_event;

	private:
		static bool is_finished(const worker_ptr & wptr) { return wptr->is_ready(); }
		void thread_func(std::atomic_bool & stop_request);

	public:
		/// meta function to obtain future type returned by submit call
		template <class Functor>
		struct future_type
		{
			typedef ext::future<std::result_of_t<std::decay_t<Functor>()>> type;
		};

	public: // execution control
		/// returns current number of workers
		unsigned get_nworkers() const;
		/// sets number of workers:
		/// if n == $current - does nothing and returns ready future
		/// if n >  $current - creates more workers and returns ready future
		/// if n <  $current - stops $current - n workers and returns future, 
		///                   which become ready when all stopped workers are completely stopped.
		ext::future<void> set_nworkers(unsigned n);

		/// stops all workers, returns future which become ready when all workers, 
		/// that were created in these thread pool at any time,
		/// are completely stopped.
		ext::future<void> stop();

	public: // job control
		/// submits task for execution, returns future representing result of execution.
		template <class Functor>
		auto submit(Functor && func) ->
			ext::future<std::result_of_t<std::decay_t<Functor>()>>;
		
		/// clears all not already executed tasks. 
		/// Associated futures status become abandoned
		void clear();

	public:
		thread_pool(unsigned nworkers = 0);
		~thread_pool() noexcept;

		thread_pool(thread_pool &&) = delete;
		thread_pool & operator =(thread_pool &&) = delete;

		thread_pool(const thread_pool &) = delete;
		thread_pool & operator =(const thread_pool &) = delete;
	};

	template <class Functor>
	auto thread_pool::submit(Functor && func) ->
		ext::future<std::result_of_t<std::decay_t<Functor>()>>
	{
		typedef std::result_of_t<std::decay_t<Functor>()> result_type;
		typedef task_impl<std::decay_t<Functor>, result_type> task_type;
		typedef ext::future<result_type> future_type;
		
		auto task = ext::make_intrusive<task_type>(std::forward<Functor>(func));
		future_type fut {task};

		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_tasks.push_back(*task.release());
		}

		m_event.notify_one();
		return fut;
	}
}
