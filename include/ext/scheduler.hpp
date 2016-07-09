#pragma once
#include <memory>
#include <utility>
#include <chrono>
#include <functional>

namespace ext
{
	/// scheduler - allows to execute task at given time point.
	/// This is interface class, implementations can use some platform-specific options, or can classical thread based.
	/// All methods must be thread-safe
	class scheduler
	{
	public:
		typedef std::function<void()> function_type;
		typedef std::chrono::steady_clock::time_point time_point;
		typedef std::chrono::steady_clock::duration   duration;

	protected:
		/// see handle class description
		struct handle_interface
		{
			virtual bool cancel() = 0;
			virtual bool is_active() const = 0;

			virtual ~handle_interface() = default;
		};

	public:
		class handle;

	public:
		/// schedules function execution at given time point
		virtual handle add(time_point tp, function_type func) = 0;
		virtual handle add(duration   rel, function_type func) = 0;
		
		/// clears all scheduled tasks
		virtual void clear() = 0;

	public:
		virtual ~scheduler() = default;
	};


	/// handle to a scheduled task. Can be used to cancel task if needed
	class scheduler::handle
	{
	private:
		std::weak_ptr<handle_interface> m_connection;

	public:
		/// cancels this task execution.
		/// returns true if task was canceled and wasn't executed/executing,
		/// otherwise returns false
		bool cancel();
		/// returns where this task is scheduled for execution.
		/// will return false if canceled or some time after executed
		bool is_active() const;

	public:
		handle() = default;
		handle(std::weak_ptr<handle_interface> e)
			: m_connection(std::move(e)) {}
	};

	inline bool scheduler::handle::cancel()
	{
		auto c = m_connection.lock();
		if (c) return c->cancel();
		else   return false;
	}

	inline bool scheduler::handle::is_active() const
	{
		auto c = m_connection.lock();
		if (c) return c->is_active(); 
		else   return false;
	}
}
