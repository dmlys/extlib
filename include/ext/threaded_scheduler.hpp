#pragma once
#include <ext/scheduler.hpp>
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ext
{
	/// scheduler implementation via background thread with priority_queue
	class threaded_scheduler : public scheduler
	{
	private:
		class entry;

		class entry_comparer
		{
		public:
			typedef bool result_type;
			bool operator()(const std::shared_ptr<entry> & e1, const std::shared_ptr<entry> & e2) const noexcept;
		};

	private:
		typedef std::priority_queue<
			std::shared_ptr<entry>,
			std::deque<std::shared_ptr<entry>>,
			entry_comparer
		> queue_type;

	private:
		queue_type m_queue;
		std::mutex m_mutex;
		std::thread m_thread;
		std::condition_variable m_newdata;
		bool stopped = false;

	private:
		void thread_func();
		void stop_thread();
		void run_passed_events();
		void add_entry(std::shared_ptr<entry> e);

		template <class Lock>
		time_point next_in(Lock & lk) const;

	public:
		virtual handle add(time_point tp, function_type func) override;
		virtual handle add(duration rel, function_type func) override;
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
