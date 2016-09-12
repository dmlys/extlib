#pragma once
#include <memory>
#include <utility>
#include <chrono>
#include <functional>
#include <ext/future.hpp>

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

		typedef ext::future<void> handle;

	public:
		/// schedules function execution at given time point
		virtual handle add(time_point tp, function_type func) = 0;
		virtual handle add(duration  rel, function_type func) = 0;
		
		/// clears all scheduled tasks
		virtual void clear() = 0;

	public:
		virtual ~scheduler() = default;
	};
}
