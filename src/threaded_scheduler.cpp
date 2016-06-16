#include <ext/threaded_scheduler.hpp>
#include <boost/thread/reverse_lock.hpp>

namespace ext
{
	class threaded_scheduler::entry :
		public handle_interface
	{
	public:
		time_point point;
		std::atomic_bool active = ATOMIC_VAR_INIT(true);
		function_type func;

	public:
		virtual bool cancel() override          { return active.exchange(false, std::memory_order_relaxed); }
		virtual bool is_active() const override { return active.load(std::memory_order_relaxed); }
	};

	inline bool threaded_scheduler::entry_comparer::operator()(const std::shared_ptr<entry> & e1, const std::shared_ptr<entry> & e2) const noexcept
	{
		// priority_queue with std::less provides constant lookup for greathest element, we need smallest
		return e1->point > e2->point;
	}

	template <class Lock>
	inline auto threaded_scheduler::next_in(Lock & lk) const -> time_point
	{
		return m_queue.empty() ? time_point::max() : m_queue.top()->point;
	}

	void threaded_scheduler::run_passed_events()
	{
		auto now = time_point::clock::now();
		std::vector<std::shared_ptr<entry>> entries_to_run;
		entries_to_run.reserve(4);
		
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			while (not m_queue.empty())
			{
				auto & top = m_queue.top();
				if (now < top->point) break;

				entries_to_run.push_back(std::move(std::move(top)));
				m_queue.pop();
			}
		}
		
		for (auto & e : entries_to_run)
		{
			// if we canceled successfully, then task was not canceled - run it.
			// suppress virtual call - not needed
			if (e->entry::cancel())
				e->func();
		}
	}

	void threaded_scheduler::thread_func()
	{
		for (;;)
		{
			run_passed_events();

			std::unique_lock<std::mutex> lk(m_mutex);
			if (stopped) return;			

			auto wait = next_in(lk);
			m_newdata.wait_until(lk, wait);
		}
	}

	void threaded_scheduler::add_entry(std::shared_ptr<entry> e)
	{
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_queue.push(std::move(e));
		}

		m_newdata.notify_one();
	}

	void threaded_scheduler::clear()
	{
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			auto dummy = std::move(m_queue);
		}

		m_newdata.notify_one();
	}

	auto threaded_scheduler::add(time_point tp, function_type func) -> handle
	{
		auto e = std::make_shared<entry>();
		e->func = std::move(func);
		e->point = tp;

		handle h(e);
		add_entry(std::move(e));
		return h;
	}

	auto threaded_scheduler::add(duration rel, function_type func) -> handle
	{
		auto e = std::make_shared<entry>();
		e->func = std::move(func);
		e->point = time_point::clock::now() + rel;

		handle h(e);
		add_entry(std::move(e));
		return h;
	}

	threaded_scheduler::threaded_scheduler()
	{
		m_thread = std::thread(&threaded_scheduler::thread_func, this);
	}

	threaded_scheduler::~threaded_scheduler()
	{
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			auto dummy = std::move(m_queue);
			stopped = true;
		}

		m_thread.join();
	}
}
