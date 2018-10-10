#include <algorithm>
#include <ext/thread_pool.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace ext
{
	thread_pool::worker::worker(thread_pool * parent)
	{
		m_parent = parent;
		m_thread = std::thread(&worker::thread_func, worker_ptr(this));
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

	void thread_pool::delayed_task_continuation::continuate(shared_state_basic * caller) noexcept
	{
		auto owner = m_owner;
		bool notify;

		if (not mark_marked())
			// thread_pool is destructed or destructing
			return;

		// remove ourself from m_delayed and add m_task to thread_pool tasks list
		{
			std::lock_guard<std::mutex> lk(owner->m_mutex);
			
			auto & list = owner->m_delayed;
			auto & delayed_count = owner->m_delayed_count;
			auto it = list.iterator_to(*this);
			list.erase(it);

			owner->m_tasks.push_back(*m_task.release());
			notify = delayed_count == 0 || --delayed_count == 0;
		}
		
		// notify thread_pool if needed
		if (notify) owner->m_event.notify_one();

		// we were removed from m_delayed - intrusive list,
		// which does not manage lifetime, decrement refcount
		release();
	}

	void thread_pool::delayed_task_continuation::abandone() noexcept
	{
		m_task->task_abandone();
		m_task = nullptr;
	}

	unsigned thread_pool::get_nworkers() const
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		return static_cast<unsigned>(m_pending);
	}

	bool thread_pool::join_worker(worker_ptr & wptr)
	{
		if (is_finished(wptr))
		{
			wptr->m_thread.join();
			return true;
		}
		else
			return false;
	}

	ext::future<void> thread_pool::set_nworkers(unsigned n)
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		if (n == m_pending) return ext::make_ready_future();
		unsigned old_size = static_cast<unsigned>(m_workers.size());

		auto first = m_workers.begin();
		auto last = m_workers.end();

		if (n > old_size - m_pending)
		{
			first = std::remove_if(last - m_pending, last, join_worker);
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

			task_ptr->task_execute();
		}
	}

	void thread_pool::clear()
	{		
		delayed_task_continuation_list delayed;
		task_list_type tasks;

		{
			std::lock_guard<std::mutex> lk(m_mutex);
			m_delayed.swap(delayed);
		}

		// cancel/take delayed tasks, see thread_pool body description
		assert(m_delayed_count == 0);
		for (auto it = delayed.begin(); it != delayed.end();)
		{
			if (not it->mark_marked())
				++m_delayed_count, ++it;
			else
			{
				auto & item = *it;
				it = delayed.erase(it);
				item.abandone();
				item.release();
			}
		}
		
		// wait until all delayed_tasks are finished, and take pending tasks
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			m_event.wait(lk, [this] { return m_delayed_count == 0; });
			tasks.swap(m_tasks);
		}
		
		tasks.clear_and_dispose([](task_base * task)
		{
			task->task_abandone();
			task->task_release();
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
		delayed_task_continuation_list delayed;

		{
			std::lock_guard<std::mutex> lk(m_mutex);
			delayed.swap(m_delayed);
			first = m_workers.begin();
			last = m_workers.end();
		}

		// stopping threads
		for (auto it = first; it != last - m_pending; ++it)
			(**it).stop_request();

		// wake threads if they are sleeping/waiting
		m_event.notify_all();

		// cancel/take delayed tasks, see thread_pool body description
		assert(m_delayed_count == 0);
		for (auto it = delayed.begin(); it != delayed.end();)
		{
			if (not it->mark_marked())
				++m_delayed_count, ++it;
			else
			{
				auto & item = *it;
				it = delayed.erase(it);
				item.abandone();
				item.release();
			}
		}

		// wait until all delayed_tasks are finished, and take pending tasks
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			m_event.wait(lk, [this] { return m_delayed_count == 0; });
			tasks.swap(m_tasks);
		}

		// abandone them
		tasks.clear_and_dispose([](task_base * task)
		{
			task->task_abandone();
			task->task_release();
		});

		// wait until threads are stopped
		for (auto it = first; it != last; ++it)
		{
			auto & worker = **it;
			worker.wait();
			worker.m_thread.join();
		}
	}
}
