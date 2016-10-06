#include <algorithm>
#include <ext/thread_pool.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace ext
{
	thread_pool::worker::worker(thread_pool * parent)
	{
		m_parent = parent;
		m_thread = std::thread(&worker::thread_func, worker_ptr(this));
		m_thread.detach();
	}
	
	void thread_pool::worker::thread_func(worker_ptr self)
	{
		self->m_parent->thread_func(self->m_stop_request);
		// mark ready on exit
		self->set_value();
	}

	bool thread_pool::worker::stop_request() noexcept
	{
		return m_stop_request.exchange(true, std::memory_order_relaxed);
	}

	unsigned thread_pool::get_nworkers() const
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		return static_cast<unsigned>(m_workers.size());
	}

	ext::future<void> thread_pool::set_nworkers(unsigned n)
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		unsigned old_size = static_cast<unsigned>(m_workers.size());
		if (n == old_size) return ext::make_ready_future();

		auto first = m_workers.begin();
		auto last = m_workers.end();

		if (n > old_size - m_pending)
		{
			first = std::remove_if(last - m_pending, last, is_finished);
			m_pending = last - first;
			m_workers.resize(n + m_pending);

			first = m_workers.begin() + old_size;
			last = m_workers.end() - m_pending;

			std::move(first, first + m_pending, last);

			for (; first != last; ++first)
				*first = ext::make_intrusive<worker>(this);

			return ext::make_ready_future();
		}
		else
		{
			first += n;
			last -= m_pending;
			m_pending += old_size - n;

			auto func = [](const worker_ptr & wptr) { return ext::future<void>(wptr); };

			auto all = ext::when_all(
				boost::make_transform_iterator(first, func),
				boost::make_transform_iterator(last, func));

			for (auto it = first; it != last; ++it)
				(**it).stop_request();

			lk.unlock();
			m_event.notify_all();

			return all.then([](auto) {});
		}
	}

	ext::future<void> thread_pool::stop()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		auto first = m_workers.begin();
		auto last = m_workers.end();

		// m_workers should not be cleared, so next call to stop will be aware of current workers
		// strictly speaking only finished workers can be cleared.

		auto func = [](const worker_ptr & wptr) { return ext::future<void>(wptr); };

		auto all = ext::when_all(
			boost::make_transform_iterator(first, func),
			boost::make_transform_iterator(last, func));

		for (auto it = first; it != last - m_pending; ++it)
			(**it).stop_request();

		lk.unlock();
		m_event.notify_all();

		return all.then([](auto) {});
	}

	void thread_pool::thread_func(std::atomic_bool & stop_request)
	{
		std::unique_lock<std::mutex> lk(m_mutex, std::defer_lock);

		for (;;)
		{
			ext::intrusive_ptr<task_base> task_ptr; 
			lk.lock();

			if (stop_request.load(std::memory_order_relaxed)) return;
			if (!m_tasks.empty()) goto avail;

		again:
			m_event.wait(lk);

			if (stop_request.load(std::memory_order_relaxed)) return;
			if (m_tasks.empty()) goto again;
			
		avail:
			task_ptr.reset(&m_tasks.front(), ext::noaddref);
			m_tasks.pop_front();

			lk.unlock();

			task_ptr->execute();
		}
	}

	void thread_pool::clear()
	{
		task_list_type actions;
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_tasks.swap(actions);
		}
		
		actions.clear_and_dispose([](task_base * task)
		{
			task->abandone();
			task->release();
		});
	}

	thread_pool::thread_pool(unsigned nworkers)
	{
		set_nworkers(nworkers);
	}

	thread_pool::~thread_pool() noexcept
	{
		// can std::atomic_memory_fence(std::memory_order_seq_cst) used?

		std::vector<worker_ptr>::iterator first, last;
		task_list_type tasks;
		{
			std::lock_guard<std::mutex> lk(m_mutex);
			tasks.swap(m_tasks);
			first = m_workers.begin();
			last = m_workers.end();
		}

		for (auto it = first; it != last - m_pending; ++it)
			(**it).stop_request();

		m_event.notify_all();

		tasks.clear_and_dispose([](task_base * task)
		{
			task->abandone();
			task->release();
		});

		for (auto it = first; it != last; ++it)
			(**it).wait();
	}
}
