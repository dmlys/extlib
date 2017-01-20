#pragma once
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <ext/intrusive_ptr.hpp>
#include <ext/future.hpp>

namespace ext
{
	/// scheduler implementation via background thread with priority_queue.
	/// Task can be submitted via submit method.
	/// For every task result of execution can be retrieved via associated future.
	/// 
	/// All methods are thread-safe
	class threaded_scheduler
	{
	public:
		typedef std::chrono::steady_clock::time_point time_point;
		typedef std::chrono::steady_clock::duration   duration;

	private:
		class task_base
		{
		public:
			time_point point;

		public:
			virtual ~task_base() = default;

			// note: addref, release are present in ext::packaged_task_impl, that can issue a conflict,
			// if signature same, but return value different - it's a error.
			// just name them differently, it's internal private class.
			virtual void task_addref()   noexcept = 0;
			virtual void task_release()  noexcept = 0;
			virtual void task_abandone() noexcept = 0;
			virtual void task_execute() = 0;

		public:
			friend inline void intrusive_ptr_add_ref(task_base * ptr) noexcept { if (ptr) ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_base * ptr) noexcept { if (ptr) ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_base * ptr) noexcept {}
		};

		template <class Functor, class ResultType>
		class task_impl :
			public task_base,
			public ext::shared_state_unexceptional<ResultType>
		{
			typedef ext::shared_state_unexceptional<ResultType> base_type;

		private:
			Functor m_functor;

		public:
			void task_addref()   noexcept override { base_type::addref(); }
			void task_release()  noexcept override { base_type::release(); }
			void task_abandone() noexcept override { base_type::release_promise(); }
			void task_execute()           override { ext::shared_state_execute(*this, m_functor); }

		public:
			task_impl(time_point tp, Functor func)
				: m_functor(std::move(func)) { task_base::point = tp; }

		public:
			friend inline void intrusive_ptr_add_ref(task_impl * ptr) noexcept { if (ptr) ptr->addref(); }
			friend inline void intrusive_ptr_release(task_impl * ptr) noexcept { if (ptr) ptr->release(); }
			friend inline void intrusive_ptr_use_count(const task_impl * ptr) noexcept {}
		};
		
		typedef ext::intrusive_ptr<task_base> task_ptr;

		class entry_comparer
		{
		public:
			typedef bool result_type;
			bool operator()(const task_ptr & t1, const task_ptr & t2) const noexcept;
		};

	private:
		typedef std::priority_queue<
			task_ptr,
			std::deque<task_ptr>,
			entry_comparer
		> queue_type;

	private:
		queue_type m_queue;
		std::thread m_thread;
		bool m_stopped = false;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_newdata;

	private:
		void thread_func();
		void run_passed_events();

		template <class Lock>
		time_point next_in(Lock & lk) const;

	public:
		template <class Functor>
		auto submit(time_point tp, Functor && func) -> 
			ext::future<std::result_of_t<std::decay_t<Functor>()>>;

		template <class Functor>
		auto submit(duration  rel, Functor && func) ->
			ext::future<std::result_of_t<std::decay_t<Functor>()>>;
		
		void clear();

	public:
		threaded_scheduler();
		~threaded_scheduler() noexcept;

		threaded_scheduler(threaded_scheduler &&) = delete;
		threaded_scheduler & operator =(threaded_scheduler &&) = delete;

		threaded_scheduler(const threaded_scheduler & ) = delete;
		threaded_scheduler & operator =(const threaded_scheduler &) = delete;		
	};

	template <class Functor>
	auto threaded_scheduler::submit(time_point tp, Functor && func) ->
		ext::future<std::result_of_t<std::decay_t<Functor>()>>
	{
		typedef std::result_of_t<std::decay_t<Functor>()> result_type;
		typedef task_impl<std::decay_t<Functor>, result_type> task_type;
		typedef ext::future<result_type> future_type;

		auto task = ext::make_intrusive<task_type>(tp, std::forward<Functor>(func));
		future_type fut {task};

		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_queue.push(std::move(task));
		}
		
		m_newdata.notify_one();
		return fut;
	}

	template <class Functor>
	inline auto threaded_scheduler::submit(duration rel, Functor && func) ->
		ext::future<std::result_of_t<std::decay_t<Functor>()>>
	{
		return submit(rel + time_point::clock::now(), std::forward<Functor>(func));
	}
}
