#pragma once
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <ext/intrusive_ptr.hpp>
#include <ext/scheduler.hpp>

namespace ext
{
	/// scheduler implementation via background thread with priority_queue
	class threaded_scheduler : public scheduler
	{
	private:
		class entry;
		typedef ext::intrusive_ptr<entry> entry_ptr;

		class entry : public ext::shared_state_unexceptional<void>
		{
			typedef ext::shared_state_unexceptional<void> base_type;
			typedef entry self_type;

		public:
			time_point point;
			function_type functor;
			// bring constructors
			using base_type::base_type;
		};
		
		class entry_comparer
		{
		public:
			typedef bool result_type;
			bool operator()(const entry_ptr & e1, const entry_ptr & e2) const noexcept;
		};

	private:
		typedef std::priority_queue<
			entry_ptr,
			std::deque<entry_ptr>,
			entry_comparer
		> queue_type;

	private:
		queue_type m_queue;
		std::mutex m_mutex;
		std::condition_variable m_newdata;
		std::thread m_thread;
		bool m_stopped = false;

	private:
		void thread_func();
		void run_passed_events();
		void add_entry(entry_ptr e);

		template <class Lock>
		time_point next_in(Lock & lk) const;

	public:
		virtual handle add(time_point tp, function_type func) override;
		virtual handle add(duration  rel, function_type func) override;
		virtual void clear() override;

	public:
		threaded_scheduler();
		~threaded_scheduler();

		threaded_scheduler(threaded_scheduler &&) = delete;
		threaded_scheduler & operator =(threaded_scheduler &&) = delete;

		threaded_scheduler(const threaded_scheduler & ) = delete;
		threaded_scheduler & operator =(const threaded_scheduler &) = delete;		
	};
}
