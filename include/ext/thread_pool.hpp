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
		typedef boost::intrusive::list_base_hook<
			boost::intrusive::link_mode<boost::intrusive::link_mode_type::normal_link>
		> hook_type;

	private:
		/// base interface for submitted tasks,
		/// tasks are hold in intrusive linked list.
		class task_base : public hook_type
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
			friend inline void intrusive_ptr_add_ref(task_base * ptr) noexcept { ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_base * ptr) noexcept { ptr->task_release(); }
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
			friend inline void intrusive_ptr_add_ref(task_impl * ptr) noexcept { ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_impl * ptr) noexcept { ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_impl * ptr) noexcept {}
		};

		class delayed_task_continuation :
			public ext::continuation_base,
			public hook_type
		{
			friend thread_pool;

			thread_pool * m_owner;
			ext::intrusive_ptr<task_base> m_task;

		public:
			void continuate() noexcept override;
			void abandone() noexcept;

		public:
			delayed_task_continuation(thread_pool * owner, ext::intrusive_ptr<task_base> task)
				: m_owner(owner), m_task(std::move(task)) {}
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
		typedef boost::intrusive::base_hook<hook_type> item_list_option;

		typedef boost::intrusive::list<
			task_base, item_list_option,
			boost::intrusive::constant_time_size<false>
		> task_list_type;

		typedef boost::intrusive::list <
			delayed_task_continuation, item_list_option,
			boost::intrusive::constant_time_size<false>
		> delayed_task_continuation_list;

	private:
		// linked list of task
		task_list_type m_tasks;

		// delayed tasks a little tricky, for every one - we create a service continuation,
		// which when fired, adds task to task_list.
		// Those can work and fire when we are destructing or destructed.
		// thred_pool lifetime should not linger on delayed_task - they should become abandoned.
		// Nevertheless we have lifetime problem.
		//
		// so we store those active service continuations in a list:
		//  - When continuation is fired it checks if it's taken:
		//    * if yes - thread_pool is gone, nothing to do;
		//    * if not - thread_pool is still there and we should add task to a list;
		// 
		// - Destructor enumerates them and marks them as taken:
		//   * if successful - service continuation is sort of cancelled and it will not access thread_pool
		//   * if not - continuation is firing right now somewhere in the middle,
		//     so destructor must wait until it finishes and then complete destruction.
		delayed_task_continuation_list m_delayed;

		// how many delayed_continuations were not "taken/cancelled" at destruction,
		// and how many we must wait - it's sort of a semaphore.
		std::size_t m_delayed_count = 0;

		// vector of worker objects, it also holds workers that are stopping.
		// vector is always partitioned by working/stopping:
		//   [0, m_pending)    - working threads
		//   [m_pending, last) - stopping threads
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
		
		template <class Future, class Functor>
		auto submit(Future future, Functor && func) ->
			ext::future<std::result_of_t<std::decay_t<Functor>(Future)>>;

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

	template <class Future, class Functor>
	auto thread_pool::submit(Future future, Functor && func) ->
		ext::future<std::result_of_t<std::decay_t<Functor>(Future)>>
	{
		auto handle = future.handle();
		auto wrapped = [func = std::forward<Functor>(func), future = std::move(future)]() mutable
		{
			return func(std::move(future));
		};

		typedef decltype(wrapped) functor_type;
		typedef std::result_of_t<functor_type()> result_type;
		typedef task_impl<functor_type, result_type> task_type;
		typedef ext::future<result_type> future_type;

		auto task = ext::make_intrusive<task_type>(std::move(wrapped));
		auto cont = ext::make_intrusive<delayed_task_continuation>(this, task);
		
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_delayed.push_back((cont.addref(), *cont.get()));
		}

		handle->add_continuation(cont.get());
		return future_type {std::move(task)};
	}
}
