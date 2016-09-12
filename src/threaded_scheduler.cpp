#include <ext/threaded_scheduler.hpp>
#include <boost/thread/reverse_lock.hpp>

namespace ext
{
	inline bool threaded_scheduler::entry_comparer::operator()(const entry_ptr & e1, const entry_ptr & e2) const noexcept
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
		entry_ptr item;

		for (;;)
		{
			{
				std::lock_guard<std::mutex> lk(m_mutex);
				if (m_queue.empty()) return;

				auto & top = m_queue.top();
				if (now < top->point) break;
			
				item = std::move(top);
				m_queue.pop();
			}
		
			ext::shared_state_execute(*item, item->functor);
		}
	}

	void threaded_scheduler::thread_func()
	{
		for (;;)
		{
			run_passed_events();

			std::unique_lock<std::mutex> lk(m_mutex);
			if (m_stopped) return;

			auto wait = next_in(lk);
			m_newdata.wait_until(lk, wait);
		}
	}

	void threaded_scheduler::add_entry(entry_ptr e)
	{
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_queue.push(std::move(e));
		}

		m_newdata.notify_one();
	}

	void threaded_scheduler::clear()
	{
		queue_type queue;
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			queue = std::move(m_queue);
		}

		while (!queue.empty())
		{
			queue.top()->release_promise();
			queue.pop();
		}

		m_newdata.notify_one();
	}

	auto threaded_scheduler::add(time_point tp, function_type func) -> handle
	{
		auto e = ext::make_intrusive<entry>();
		e->functor = std::move(func);
		e->point = tp;

		handle h {e};
		add_entry(std::move(e));
		return h;
	}

	auto threaded_scheduler::add(duration rel, function_type func) -> handle
	{
		auto e = ext::make_intrusive<entry>();
		e->functor = std::move(func);
		e->point = time_point::clock::now() + rel;

		handle h {e};
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
			m_stopped = true;
			
			while (!m_queue.empty())
			{
				m_queue.top()->release_promise();
				m_queue.pop();
			}
		}
		
		m_newdata.notify_one();
		m_thread.join();
	}
}
