#pragma once
// author: Dmitry Lysachenko
// date: Saturday 20 august 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <ciso646> // for not, and, etc
#include <climits> // for CHAR_BIT
#include <cstdint> // for std::uintptr_t
#include <cassert>

#include <atomic>
#include <chrono>   // for wait_* operations
#include <tuple>    // used by when_all/when_any
#include <utility>
#include <type_traits>
#include <system_error> // for std::error_code
#include <exception>    // for std::exception_ptr and others

#include <vector>   // used in when_any_result
#include <iterator> // iterator_traits

#include <thread>   // for ext::async(ext::launch::async, ...)
#include <mutex>    // for wait_* operations
#include <condition_variable>

#include <ext/config.hpp>  // for EXT_UNREACHABLE
#include <ext/utility.hpp> 
#include <ext/intrusive_ptr.hpp>
#include <ext/try_reserve.hpp>

namespace ext
{
	/// status of a future
	enum class future_status : unsigned
	{
		ready,     // the shared state is ready
		deferred,  // the shared state contains a deferred function, so the result will be computed only when explicitly requested

		timeout,   // the shared state did not become ready before specified timeout duration has passed
	};

	/// status of a promise, extension
	enum class future_state : unsigned
	{
		unsatisfied,  // is unsatisfied, waiting for value
		deffered,     // future is deffered. It has no value yet, but will become value/exception on get call
		value,        // holds a value
		exception,    // holds an exception
		cancellation, // promise was cancelled though an associated future
		abandonned,   // promise was abandoned
	};

	/// launch policy
	enum class launch : unsigned
	{
		async    = 1,
		deferred = 2,
	};

	/// future error conditions(strictly error codes)
	enum class future_errc
	{
		broken_promise             = 1,
		future_already_retrieved   = 2,
		promise_already_satisfied  = 3,
		no_state                   = 4,
		cancelled                  = 5,
	};

	const std::error_category & future_category();

	inline std::error_condition make_error_condition(future_errc err) { return {static_cast<int>(err), future_category()}; }
	inline std::error_code      make_error_code(future_errc err)      { return {static_cast<int>(err), future_category()}; }

	/// The class ext::future_error defines an exception object, used by promise, future, and other types.
	/// Similar to std::system_error, this exception carries an error code compatible with std::error_code.
	class future_error : public std::logic_error
	{
		std::error_code m_err;
	
	public:
		future_error(const std::error_code & ec) noexcept;

		const std::error_code & code() const noexcept { return m_err; }
		const char * what() const noexcept override;
	};

	/// Forward declarations
	template <class Type> class future;
	template <class Type> class shared_future;
	template <class Type> class promise;
	template <class Type> class packaged_task;

	// type_traits helpers
	template <class Type> struct is_future_type : std::false_type {};
	template <class Type> struct is_future_type<ext::future<Type>> : std::true_type {};
	template <class Type> struct is_future_type<ext::shared_future<Type>> : std::true_type {};

	template <class ... Types>
	struct is_future_types;

	template <class Type> struct is_future_types<Type> :
		is_future_type<Type> {};
	
	template <class Head, class ... Tail>
	struct is_future_types<Head, Tail...> :
		std::integral_constant<bool, is_future_type<Head>::value && is_future_types<Tail...>::value> {};


	/// result of whan_any call
	template <class Sequence>
	struct when_any_result
	{
		std::size_t index;
		Sequence futures;
	};


	void init_future_library(unsigned waiter_slots = std::thread::hardware_concurrency() * 4);
	void free_future_library();


	/// returns satisfied future immediately holding val
	template <class Type>
	future<std::decay_t<Type>> make_ready_future(Type && val);


	template<class Function, class ... Args>
	auto async(ext::launch policy, Function && f, Args && ... args) ->
		future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>;

	template<class Function, class ... Args>
	auto async(Function && f, Args && ... args) ->
		future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>
	{
		constexpr ext::launch pol = static_cast<ext::launch>(
			static_cast<int>(ext::launch::async) |
			static_cast<int>(ext::launch::deferred)
		);

		return async(pol, std::forward<Function>(f), std::forward<Args>(args)...);
	}


	template <class InputIterator>
	auto when_any(InputIterator first, InputIterator last) ->
		std::enable_if_t<
			is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
			ext::future<when_any_result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>
		>;

	template <class ... Futures>
	auto when_any(Futures && ... futures) ->
		std::enable_if_t<
			is_future_types<std::decay_t<Futures>...>::value,
			ext::future<when_any_result<std::tuple<std::decay_t<Futures>...>>>
		>;

	template <class InputIterator>
	auto when_all(InputIterator first, InputIterator last) ->
		std::enable_if_t<
			is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
			ext::future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>
		>;

	template <class ... Futures>
	auto when_all(Futures && ... futures) ->
		std::enable_if_t<
			is_future_types<std::decay_t<Futures>...>::value,
			ext::future<std::tuple<std::decay_t<Futures>...>>
		>;

	/// Ideally i want to specify abstract virtual interfaces for future, promise, packaged_task.
	/// ext::future, ext::promise, ext::packaged_task would be front-end classes used by clients.
	/// This library would provide implementation of those interfaces via:
	/// * shared_state_basic - type-independent implementation: refcount, wait, valid, etc
	/// * shared_state - type-dependent implementation: get, set_value, etc
	/// but clients would be able to implement own if they need so.
	/// 
	/// It's very hard to specify future interface(ifuture) for 2 reasons:
	/// * this one minor:
	/// 
	///   if ifuture is template of Type, then shared_state_basic parametrized by ifuture<Type>(or even other interface).
	///   this would lead to duplication of type-independent methods for each specialization.
	///   Another option - use multiple inheritance(possibly with virtual base classes) - this leads to slightly increased object size
	/// 
	///   ifuture could be not a template class -> and get method would return void *.
	///   Parametrization would be done by front-end classes: future, shared_duture.
	///   While ifuture is loosly typed, ext::future, ext::shared_future takes care of this.
	///   
	/// * major:
	/// 
	///   it's hard to specify 'then' method:
	///   it's takes some functor template as argument, and returns future of RetType,
	///   where RetType is return type of argument-functor.
	///   
	///   What signature should method 'then' have?
	///   It could return typeless ifuture, and ext::future, ext::shared_future takes care of typization.
	///   But how functor should be passed? std::function<RetType()>? What RetType should be?
	///   It could be std::function<void *()> -> now method 'then' could create packeged_task<void *()> and return it.
	///   But it's state is of type void *, not Type. Where Type instance will be stored?
	///   It can be placed into std::function<void *()>, but Type can be not default constactable - can be solved too by aligned staorage.
	///  
	///   At this point we need a special object which provides space for holding Type and type erasing for both: functor and Type instance.
	/// 
	/// For now implement without interfaces. ext::future would hold a pointer to shared_state<Type>.
	/// Method 'then' will be template function -> thus can easily create packaged_task<RetType()>

	/// Shared state implementation is divided between several classes:
	/// * shared_state_basic - type independent part, implementation of promise state, continuations
	/// * shared_state - type dependent part, have space for object, exception pointer
	/// * shared_state_unexceptional - same as shared_state, but does not allow set_exception and thus not storing std::exception_ptr,
	///                                can be used by asynchronous task not throwing exceptions
	/// Other classes are:
	/// * continuation_task   - shared_state derived class used for continuations
	/// * continuation_waiter - shared_state derived class used as continuations which implements wait functionality
	/// * packaged_task_impl  - packaged_task implementation
	/// * when_any/all_task   - classes for implementing when_all/any functions

	class shared_state_basic;
	template <class Type> class shared_state;
	template <class Type> class shared_state_unexceptional;

	class continuation_waiter;
	template <class>        class packaged_task_base;
	template <class, class> class packaged_task_impl;
	template <class, class> class deferred_task_impl;
	template <class, class> class continuation_task;
	template <class, class> class deferred_continuation_task;

	class when_any_task_continuation;
	class when_all_task_continuation;

	template <class> class when_any_task;
	template <class> class when_all_task;
	
	/// shared_state_basic type independent part, implementation of promise state, continuations
	/// 
	/// Design goals:
	/// * we must hold information of promise state to prevent multiple value submission.
	/// * future status - ready, cancelled, broken promise(promise destroyed without setting value).
	/// * continuations - those can be submitted concurrently via multiple shared_futures to the same shared state.
	/// * waiting - future can be waited for becoming ready, including concurrently via shared_futures
	/// 
	///
	/// Waiting means mutex and condition_variable.
	/// On the other hand - I want this object to be as small as possible, with reasonable complexity.
	/// I don't want it to always hold mutex and condition variable.
	/// 
	/// Also while future goal is to be used in multi-thread environment -
	/// 'wait' and 'then' calls would not have big concurrency in normal program.
	/// Normally user puts few continuations and that's all, wait calls also not so frequent.
	///
	/// To handle continuations we have sort of intrusive slist of continuations.
	/// We control implementation of 'then' method and how continuation are created.
	/// Each continuation is heap-allocated object derived from us(shared_state_basic)
	/// and holds atomic pointer to next continuation.
	/// 
	/// Waiting is also implemented as continuation object, thus avoiding holding of mutex and condition_variable in shared_state_basic itself.
	/// continuation_waiter is acquired from waiter pool, when wait request is issued.
	/// It's shared if there multiple wait calls, and released when there are none.
	/// There is always no more than 1 continuation_waiter,
	/// and if it's present - it's always at the head of continuation chain.
	/// This allows easy sharing on continuation_waiter and removing it, when not needed.
	/// 
	///
	/// While atomic pointers are used, slist is NOT lock-free.
	/// Lock-free implementation would be very hard, if possible.
	/// 
	/// Instead we use pointer locking:
	/// locking pointer - is a spin-lock loop trying to set low bit to 1, while expecting it to be 0.
	/// Assuming not high concurrency(see above) it should posses no problems.
	/// 
	/// Also when shared_state become ready, all new submitted continuations must be executed immediately.
	/// There is a race condition here. While we are adding new continuation - we can become ready.
	/// To prevent this, continuation submission and transition to fulfilled state must be done in single atomic step.
	/// This is done by locking head and replacing it with special ready value.
	/// 
	/// Simplified algorithms:
	/// Wherever there is a waiting request:
	/// 1. lock slist head.
	/// 2. if special ready value - waiting is not needed - return.
	/// 3. head is a waiter continuation:
	///    * if so, it's refcount is increased and object returned to caller(caller waits on it).
	///    * if not - waiter object is acquired from pool and placed as new head,
	///      after that it's returned to caller.
	/// 4. unlock head
	/// 
	/// Whenever new continuation is added - new object is created,
	/// 1. lock slist head.
	/// 2. if special ready value - execute continuation immediately and return.
	/// 3. checked if it is continuation_waiter:
	///    * if so - continuation inserted after head,
	///    * if not - continuation added as new head.
	/// 4. unlock head
	/// 
	/// we hold state as atomic std::uintptr_t:
	/// * 0        - future_status::ready
	/// * 0xFF..FF - waiting result and no continuations, locked
	///   0xFF..FE - waiting result and no continuations, unlocked, also initial value
	/// * other    - waiting result and this is head of slist(pointer to next continuation)
	/// 
	/// When shared_state becomes ready, state changes to future_status::ready
	/// and previous val is head of continuation slist which must be executed.
	///
	/// promise state is only checked by set_value path -> we can use sort of hierarchical lock,
	/// first change promise state, than change future state.
	class shared_state_basic
	{
		typedef shared_state_basic self_type;

	protected:
		typedef shared_state_basic continuation_type;

		static constexpr unsigned future_retrived = 1u << (sizeof(unsigned) * CHAR_BIT - 1);
		static constexpr unsigned future_uncancellable = future_retrived >> 1u;
		static constexpr unsigned status_mask = ~(future_retrived | future_uncancellable);

		// future state, see description above.
		static constexpr std::uintptr_t ready = 0;
		// mask to extract lock state
		static constexpr std::uintptr_t lock_mask = 1;

	protected:
		/// object lifetime reference counter
		std::atomic_uint m_refs = ATOMIC_VAR_INIT(1);
		/// state of promise, unsatisfied, satisfied by: value, exception, cancellation, ...
		std::atomic<unsigned> m_promise_state = ATOMIC_VAR_INIT(static_cast<unsigned>(future_state::unsatisfied));
		/// state and head of continuations slist
		std::atomic_uintptr_t m_fstnext = ATOMIC_VAR_INIT(~lock_mask);

	protected:
		static bool is_waiter(continuation_type * ptr) noexcept;
		static bool is_continuation(std::uintptr_t fstate) noexcept { return fstate < ~lock_mask; }
		static future_state pstatus(unsigned promise_state) noexcept { return static_cast<future_state>(promise_state & status_mask); }

		/// re-inits this shared_state_basic as deffered, should be called from constructor of derived class.
		/// NOTE: probably shared_state_basic constructor initializing this class as deffered is better, 
		///       but than it's must be forwarded by all derived classes...
		void init_deffered() noexcept
		{ this->m_promise_state.store(static_cast<unsigned>(future_state::deffered), std::memory_order_relaxed); }

	public:
		/// locks pointer by setting lowest bit in spin-lock loop.
		/// returns value which pointer actually has.
		/// has std::memory_order_acquire semantics.
		static auto lock_ptr(std::atomic_uintptr_t & ptr) noexcept -> std::uintptr_t;
		/// unlocks pointer, by reseting lowest bit, pointer must be locked before
		/// has std::memory_order_release semantics.
		static void unlock_ptr(std::atomic_uintptr_t & ptr) noexcept;
		/// unlocks pointer and sets it to newval, pointer must be locked before
		/// has std::memory_order_release semantics.
		static void unlock_ptr(std::atomic_uintptr_t & ptr, std::uintptr_t newval) noexcept;

		/// set fstnext status to ready and returns continuation chain.
		/// has std::memory_order_acq_rel semantics.
		static std::uintptr_t signal_future(std::atomic_uintptr_t & fstnext) noexcept;
		/// attaches continuation to a continuation list with head, increments refcount.
		/// after continuation fires - refcount decrement.
		/// returns false if shared_state was ready and continuation fired immediately
		static bool attach_continuation(std::atomic_uintptr_t & head, continuation_type * continuation) noexcept;
		/// runs continuations slist pointed by addr, checks if addr is_continuation.
		/// typically should be called after signal_future.
		/// for each item in list continuate and release are called, than next item is taken from m_fstnext.
		/// if continuate some how affects item pointed by m_fstnext it probably should set it to ready or not_a_continuation.
		static void run_continuations(std::uintptr_t addr) noexcept;
		/// acquires waiter from continuation list, it it has it,
		/// or acquires it from waiter objects pool and attaches it to continuation list.
		static auto accquire_waiter(std::atomic_uintptr_t & head) -> continuation_waiter *;
		/// release waiter, it there no more usages - detaches to from continuation list,
		/// and release it to waiter objects pool. waiter must be acquired by accquire_waiter call.
		static void release_waiter(std::atomic_uintptr_t & head, continuation_waiter * waiter);

	public:
		/// changes this state from unsatisfied/deffered to satisfied with reason.
		/// if state was already satisfied - throws promise_already_satisfied,
		/// unless previous state was cancelled - in that case returns false.
		bool satisfy_check_promise(future_state reason);
		/// changes this state from unsatisfied/deffered to satisfied with reason.
		/// if state was already satisfied - returns false, noexcept.
		bool satisfy_promise(future_state reason) noexcept;

	public:
		/// set future status to ready and runs continuations
		virtual void set_future_ready() noexcept;
		/// attaches continuation to the internal continuation, increments refcount.
		/// after continuation fires - refcount decrement.
		/// returns false if shared_state was ready and continuation fired immediately
		virtual bool add_continuation(continuation_type * continuation) noexcept;
		/// acquires waiter from internal continuation list, it it has it,
		/// or acquires it from waiter objects pool and attaches it to internal continuation list.
		virtual auto accquire_waiter() -> continuation_waiter *;
		/// release waiter, it there no more usages - detaches to from internal continuation list,
		/// and release it to waiter objects pool
		virtual void release_waiter(continuation_waiter * waiter);

	public:
		/// continuation support, must be implemented only by classes used as continuations:
		/// shared_state_basic only uses continuation_waiter and continuation_task.
		/// Others - should ignore this method, default implementation calls std::terminate.
		virtual void continuate() noexcept { std::terminate(); }

		/// special method for when_all, when_any support
		/// index - index of satisfied shared_state. Used only by when_any 
		virtual void notify_satisfied(std::size_t index) noexcept { std::terminate(); }

	public:
		/// refcount part, use_count is for intrusive_ptr
		/* virutal */ unsigned addref(unsigned n = 1);
		/* virutal */ unsigned release();
		/* virutal */ unsigned use_count() const;
		
		/// marks future retrieval from shared state
		void mark_retrived();
		/// marks future cannot be cancelled(cancel call would return false),
		/// but future is still not fulfilled.
		/// returns true if successful. false if shared_state already holds some result(cancellation, value, ...)
		/// or it was already marked uncancellable
		bool mark_uncancellable() noexcept;

		/// status of shared state
		future_state status() const noexcept { return pstatus(m_promise_state.load(std::memory_order_relaxed)); }

		/// status shortcuts
		bool is_pending() const noexcept    { return status() == future_state::unsatisfied; }
		bool is_ready() const noexcept      { return status() != future_state::unsatisfied; }
		bool is_abandoned() const noexcept  { return status() == future_state::abandonned; }
		bool is_cancelled() const noexcept  { return status() == future_state::cancellation; }
		bool is_deffered() const noexcept   { return status() == future_state::deffered; }
		bool has_value() const noexcept     { return status() == future_state::value; }
		bool has_exception() const noexcept { return status() == future_state::exception; }

		/// Cancellation request. If request successfully fulfilled - returns true,
		/// If future is is already fulfilled - returns false.
		/// If implementation does not support it - it can throw an exception.
		/// Promise which comes with this library - supports cancellation.
		virtual bool cancel() noexcept;

		/// Called by ext::promise on destruction,
		/// if value wasn't satisfied - set broken promise status
		virtual void release_promise() noexcept;

		/// Blocks until result becomes available,
		/// The behavior is undefined if valid() == false before the call to this function.
		virtual void wait() const;
		/// Waits for the result to become available.
		/// Blocks until specified timeout_duration has elapsed or the result becomes available,
		/// whichever comes first.Returns value identifies the state of the result.
		/// The behavior is undefined if valid() == false before the call to this function.
		virtual future_status wait_for(std::chrono::steady_clock::duration timeout_duration) const;
		/// Waits for the result to become available.
		/// Blocks until specified timeout_time has been reached or the result becomes available,
		/// whichever comes first.Returns value identifies the state of the result.
		/// The behavior is undefined if valid() == false before the call to this function.
		virtual future_status wait_until(std::chrono::steady_clock::time_point timeout_point) const;

		// set_value, set_exception are implemented in shared_state
		
		/// type-erased get method, returns pointer to a stored value, for void future returns nullptr
		/// for reference specializations returns stored pointer.
		virtual void * get_ptr() = 0;

	public:
		/// typed return, type is specified explicitly, it uses get_ptr internally and necessary casts
		template <class Type> Type get();

		/// Continuation support, when future becomes fulfilled,
		/// either by result(normal or exception) becoming available,
		/// or cancellation request was made on this future.
		///
		/// Continuation is executed in the context of this future, immediately after result becomes available
		template <class Type, class Functor>
		auto add_unique_continuation(Functor && continuation) ->
			ext::future<std::result_of_t<std::decay_t<Functor>(ext::future<Type>)>>;

		/// Continuation support, when future becomes fulfilled,
		/// either by result(normal or exception) becoming available,
		/// or cancellation request was made on this future.
		///
		/// Continuation is executed in the context of this future, immediately after result becomes available
		template <class Type, class Functor>
		auto add_shared_continuation(Functor && continuation) ->
			ext::future<std::result_of_t<std::decay_t<Functor>(ext::shared_future<Type>)>>;

	public:
		shared_state_basic() = default;
		virtual ~shared_state_basic() = default;

		shared_state_basic(shared_state_basic &&) = delete;
		shared_state_basic & operator = (shared_state_basic &&) = delete;
	};


	inline void intrusive_ptr_add_ref(shared_state_basic * ptr) { if (ptr) ptr->addref(); }
	inline void intrusive_ptr_release(shared_state_basic * ptr) { if (ptr) ptr->release(); }
	inline unsigned intrusive_ptr_use_count(shared_state_basic * ptr) { return ptr ? ptr->use_count() : 0; }
	

	/// base class for service continuations, 
	/// classes inheriting this class are service continuations and should not be seen to user.
	/// they do not have result, and implement some logic on continuate method.
	/// also they do not affect m_fstnext member leaving it as is for run_continuations method.
	class continuation_base : public shared_state_basic
	{
	public:
		void * get_ptr() override { throw std::logic_error("continuation_base::get_ptr unexpected"); };
		void set_future_ready() noexcept override {};
		void continuate() noexcept override = 0;
	};

	/// shared_state - type dependent part, have space for object, exception pointer.
	/// implements get, set_value, set_exception methods, heavily based on shared_state_basic
	/// @Param Type - type of channel, this shared state will hold value of this type
	template <class Type>
	class shared_state : public shared_state_basic
	{
		typedef shared_state_basic   base_type;
		typedef shared_state<Type>   self_type;

	public:
		typedef Type value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	protected:
		/// shared data, type or exception
		/// type is held in m_promise_state
		union
		{
			value_type          m_val;
			std::exception_ptr  m_exptr;
		};

	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(const value_type & val);
		void set_value(value_type && val);
		/// fulfills promise and sets exception result ex
		void set_exception(std::exception_ptr ex);

	public:
		shared_state() noexcept;
		~shared_state() noexcept;

		shared_state(shared_state &&) = delete;
		shared_state & operator = (shared_state &&) = delete;
	};

	/// shared_state reference specialization
	template <class Type>
	class shared_state<Type &> : public shared_state_basic
	{
		typedef shared_state_basic   base_type;
		typedef shared_state<Type &> self_type;

	public:
		typedef Type value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	protected:
		/// shared data, type or exception
		/// type is held in m_promise_state
		union
		{
			value_type *        m_val;
			std::exception_ptr  m_exptr;
		};

	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(value_type & val);
		/// fulfills promise and sets exception result ex
		void set_exception(std::exception_ptr ex);

	public:
		shared_state() noexcept;
		~shared_state() noexcept;

		shared_state(shared_state &&) = delete;
		shared_state & operator = (shared_state &&) = delete;
	};

	/// shared_state void specialization
	template <>
	class shared_state<void> : public shared_state_basic
	{
		typedef shared_state_basic   base_type;
		typedef shared_state<void>   self_type;

	public:
		typedef void value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	protected:
		/// shared data, type or exception
		/// type is held in m_promise_state
		std::exception_ptr m_exptr;

	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(void);
		/// fulfills promise and sets exception result ex
		void set_exception(std::exception_ptr ex);

	public:
		shared_state() noexcept;
		~shared_state() noexcept;

		shared_state(shared_state &&) = delete;
		shared_state & operator = (shared_state &&) = delete;
	};


	/// shared_state - type dependent part, have space for object, but does not support setting exceptions.
	/// implements get, set_value methods, heavily based on shared_state_basic
	/// @Param Type - type of channel, this shared state will hold value of this type
	template <class Type>
	class shared_state_unexceptional : public shared_state_basic
	{
		typedef shared_state_basic                 base_type;
		typedef shared_state_unexceptional<Type>   self_type;

	public:
		typedef Type value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	protected:
		/// shared data, type or exception
		/// type is held in m_promise_state
		union
		{
			value_type m_val;
		};

	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(const value_type & val);
		void set_value(value_type && val);

	public:
		shared_state_unexceptional() noexcept;
		~shared_state_unexceptional() noexcept;

		shared_state_unexceptional(shared_state_unexceptional &&) = delete;
		shared_state_unexceptional & operator = (shared_state_unexceptional &&) = delete;
	};


	/// shared_state_unexceptional reference specialization
	template <class Type>
	class shared_state_unexceptional<Type &> : public shared_state_basic
	{
		typedef shared_state_basic                  base_type;
		typedef shared_state_unexceptional<Type &>  self_type;

	public:
		typedef Type value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	protected:
		/// shared data, type or exception
		/// type is held in m_promise_state
		value_type * m_val;

	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(value_type & val);

	public:
		shared_state_unexceptional() noexcept = default;
		~shared_state_unexceptional() noexcept = default;

		shared_state_unexceptional(shared_state_unexceptional &&) = delete;
		shared_state_unexceptional & operator = (shared_state_unexceptional &&) = delete;
	};

	/// shared_state_unexceptional void specialization
	template <>
	class shared_state_unexceptional<void> : public shared_state_basic
	{
		typedef shared_state_basic                base_type;
		typedef shared_state_unexceptional<void>  self_type;

	public:
		typedef void value_type;

	public:
		using base_type::status;
		using base_type::satisfy_check_promise;
		using base_type::set_future_ready;
		
	public:
		void * get_ptr() override;
		/// fulfills promise and sets shared result val
		void set_value(void);

	public:
		shared_state_unexceptional() noexcept = default;
		~shared_state_unexceptional() noexcept = default;

		shared_state_unexceptional(shared_state_unexceptional &&) = delete;
		shared_state_unexceptional & operator = (shared_state_unexceptional &&) = delete;
	};


	/// Implements continuation used for waiting by shared_state_basic.
	/// Derived from shared_state_basic, sort of recursion
	class continuation_waiter : public continuation_base
	{
	private:
		std::mutex m_mutex;
		std::condition_variable m_var;
		bool m_ready = false;

	public:
		/// waiting functions: waits until object become ready by execute call
		void wait_ready() noexcept;
		bool wait_ready(std::chrono::steady_clock::time_point timeout_point) noexcept;
		bool wait_ready(std::chrono::steady_clock::duration   timeout_duration) noexcept;

	public:
		/// fires condition_variable, waking any waiting thread
		void continuate() noexcept override;
		/// reset waiter, after that it can be used again
		void reset() noexcept;
	};


	template <class Ret, class ... Args>
	class packaged_task_base<Ret(Args...)> : public shared_state<Ret>
	{
		typedef packaged_task_base           self_type;
		typedef shared_state<Ret>            base_type;

	public:
		virtual bool valid() const noexcept = 0;
		virtual void execute(Args ... args) noexcept = 0;
		virtual self_type * reset() = 0;
	};

	/// The packaged_task_impl is back-end class for packaged_task,
	/// it wraps Callable target (function, lambda expression, bind expression, or another function object)
	/// so that it can be invoked asynchronously.
	/// 
	/// Its return value or exception thrown is stored in a shared state
	/// which can be accessed through ext::future objects.
	template <class Functor, class Ret, class ... Args>
	class packaged_task_impl<Functor, Ret(Args...)> : public packaged_task_base<Ret(Args...)>
	{
		typedef packaged_task_impl                 self_type;
		typedef packaged_task_base<Ret(Args...)>   base_type;

	public:
		typedef Functor functor_type;

	protected:
		functor_type m_functor;

	public:
		bool valid() const noexcept override { return true; /*static_cast<bool>(m_functor);*/ }
		void execute(Args ... args) noexcept override;
		self_type * reset() override;

	public:
		packaged_task_impl() noexcept = default;
		packaged_task_impl(functor_type f) noexcept
			: m_functor(std::move(f)) {}
	};

	/// deferred_task_impl is packaged_task_impl.
	/// It calls stored functor on first get/wait request, used in ext::async call.
	template <class Functor, class Ret>
	class deferred_task_impl<Functor, Ret()> : public packaged_task_impl<Functor, Ret()>
	{
		typedef deferred_task_impl                    self_type;
		typedef packaged_task_impl<Functor, Ret()>    base_type;

	public:
		void wait() const override;
		future_status wait_for(std::chrono::steady_clock::duration timeout_duration) const override  { return future_status::deferred; }
		future_status wait_until(std::chrono::steady_clock::time_point timeout_point) const override { return future_status::deferred; }

	public:
		deferred_task_impl() noexcept { this->init_deffered(); }
		deferred_task_impl(Functor func) noexcept : base_type(std::move(func)) { this->init_deffered(); }
	};

	/// implements continuations(future::then, shread_future::then)
	template <class Functor, class Type>
	class continuation_task : public packaged_task_impl<Functor, Type()>
	{
		typedef continuation_task                    self_type;
		typedef packaged_task_impl<Functor, Type()>  base_type;

	protected:
		typedef shared_state_basic continuation_type;
		using base_type::lock_mask;
		
		using base_type::signal_future;
		using base_type::attach_continuation;
		using base_type::accquire_waiter;
		using base_type::release_waiter;
		using base_type::run_continuations;

	protected:
		/// like shared_state_basic::m_fsnext, see shared_state_basic class description.
		/// But this is continuation chain of this continuation_task.
		/// It's needed because user can wait on this future, or even cancel it.
		/// This should not affect parent future.
		std::atomic_uintptr_t m_task_next = ATOMIC_VAR_INIT(~lock_mask);

	protected:
		/// set future status to ready and runs continuations
		void set_future_ready() noexcept override;
		/// attaches continuation to the internal continuation, increments refcount.
		/// after continuation fires - refcount decrement.
		/// returns false if shared_state was ready and continuation fired immediately
		bool add_continuation(continuation_type * continuation) noexcept override
		{ return attach_continuation(m_task_next, continuation); }
		/// acquires waiter from internal continuation list, it it has it,
		/// or acquires it from waiter objects pool and attaches it to internal continuation list.
		auto accquire_waiter() -> continuation_waiter * override
		{ return accquire_waiter(m_task_next); }
		/// release waiter, it there no more usages - detaches to from internal continuation list,
		/// and release it to waiter objects pool
		void release_waiter(continuation_waiter * waiter) noexcept override
		{ return release_waiter(m_task_next, waiter); }

	public:
		void continuate() noexcept override;

	public:
		// inherit constructors
		using base_type::base_type;
	};

	/// implements continuations(future::then, shread_future::then) for deferred futures
	template <class Functor, class Type>
	class deferred_continuation_task : public continuation_task<Functor, Type>
	{
		typedef deferred_continuation_task         self_type;
		typedef continuation_task<Functor, Type>   base_type;

	public:
		void wait() const override;
		future_status wait_for(std::chrono::steady_clock::duration timeout_duration) const override { return future_status::deferred; }
		future_status wait_until(std::chrono::steady_clock::time_point timeout_point) const override { return future_status::deferred; }

	public:
		deferred_continuation_task() noexcept { this->init_deffered(); }
		deferred_continuation_task(Functor func) noexcept : base_type(std::move(func)) { this->init_deffered(); }
	};
	
	/// shared state returned by whan_any call.
	/// Type is vector<future> for tuple<future...>
	template <class Type>
	class when_any_task : public shared_state_unexceptional<Type>
	{
		template <class InputIterator>
		friend auto when_any(InputIterator first, InputIterator last) ->
			std::enable_if_t<
				is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
				ext::future<when_any_result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>
			>;

		template <class ... Futures>
		friend auto when_any(Futures && ... futures) ->
			std::enable_if_t<
				is_future_types<std::decay_t<Futures>...>::value,
				ext::future<when_any_result<std::tuple<std::decay_t<Futures>...>>>
			>;

	private:
		typedef shared_state_unexceptional<Type>  base_type;
		typedef when_any_task                     self_type;

	protected:
		using typename base_type::value_type;
		using base_type::m_val;
		using base_type::m_promise_state;

	public:
		//void set_value(value_type && val) override { std::terminate(); }
		//void set_value(const value_type & val) override { std::terminate(); }
		void notify_satisfied(std::size_t index) noexcept override;

	public:
		when_any_task(Type && val) { new (&m_val) Type(std::move(val)); }
		~when_any_task() noexcept { m_promise_state.store(static_cast<unsigned>(future_state::value), std::memory_order_relaxed); }
	};

	template <class Type>
	class when_all_task : public shared_state_unexceptional<Type>
	{
		template <class InputIterator>
		friend auto when_all(InputIterator first, InputIterator last) ->
			std::enable_if_t<
				is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
				ext::future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>
			>;

		template <class ... Futures>
		friend auto when_all(Futures && ... futures) ->
			std::enable_if_t<
				is_future_types<std::decay_t<Futures>...>::value,
				ext::future<std::tuple<std::decay_t<Futures>...>>
			>;

	private:
		typedef shared_state_unexceptional<Type>  base_type;
		typedef when_all_task                     self_type;

	protected:
		using base_type::m_val;
		using base_type::m_promise_state;

	protected:
		std::atomic_size_t m_count;

	public:
		//void set_value(value_type && val) override { std::terminate(); }
		//void set_value(const value_type & val) override { std::terminate(); }
		void notify_satisfied(std::size_t index) noexcept override;

	public:
		when_all_task(Type && val, std::size_t count) : m_count(count) { new (&m_val) Type(std::move(val)); }
		~when_all_task() noexcept { m_promise_state.store(static_cast<unsigned>(future_state::value), std::memory_order_relaxed); }
	};

	/// special continuation task.
	/// Used as continuation into argument future, notifies parent when_any_task	
	class when_any_task_continuation : public continuation_base
	{
		typedef shared_state_unexceptional<void>  base_type;
		typedef when_any_task_continuation        self_type;
		typedef shared_state_basic                parent_task_type;
	
	protected:
		ext::intrusive_ptr<parent_task_type> m_parent;
		std::size_t m_index;

	public:
		void continuate() noexcept override { m_parent->notify_satisfied(m_index); }

	public:
		when_any_task_continuation(ext::intrusive_ptr<parent_task_type> parent, std::size_t index) noexcept
			: m_parent(std::move(parent)), m_index(index) {}
	};

	class when_all_task_continuation : public continuation_base
	{
		typedef shared_state_unexceptional<void>  base_type;
		typedef when_all_task_continuation        self_type;
		typedef shared_state_basic                parent_task_type;

	protected:
		ext::intrusive_ptr<parent_task_type> m_parent;

	public:
		void continuate() noexcept override { m_parent->notify_satisfied(0); }

	public:
		when_all_task_continuation(ext::intrusive_ptr<parent_task_type> parent) noexcept
			: m_parent(std::move(parent)) {}
	};

	/************************************************************************/
	/*            shared_state_basic lifetime                                */
	/************************************************************************/
	inline unsigned shared_state_basic::addref(unsigned n)
	{
		return m_refs.fetch_add(n, std::memory_order_relaxed);
	}
	
	inline unsigned shared_state_basic::release()
	{
		auto ref = m_refs.fetch_sub(1, std::memory_order_release);
		if (ref == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);
			delete this;
		}

		return --ref;
	}

	inline unsigned shared_state_basic::use_count() const
	{
		return m_refs.load(std::memory_order_relaxed);
	}


	inline bool shared_state_basic::is_waiter(continuation_type * ptr) noexcept
	{
		return typeid(*ptr) == typeid(continuation_waiter);
	}

	inline void shared_state_basic::mark_retrived()
	{
		auto prev = m_promise_state.fetch_or(future_retrived, std::memory_order_relaxed);
		if (prev & future_retrived)
			throw future_error(make_error_code(future_errc::future_already_retrieved));
	}

	template <class Type>
	inline Type shared_state_basic::get()
	{
		typedef std::conditional_t<
			std::is_reference<Type>::value,
			Type &,     // if reference return as from reference
			Type &&     // if val, cast to rvalue, so we can move
		> cast_type;

		typedef std::add_pointer_t<
			std::remove_reference_t<Type>
		> pointer_type;

		auto * val_ptr = reinterpret_cast<pointer_type>(get_ptr());
		return static_cast<cast_type>(*val_ptr);
	}

	template <>
	inline void shared_state_basic::get()
	{
		get_ptr();
	}

	template <class Type, class Functor>
	auto shared_state_basic::add_unique_continuation(Functor && continuation) ->
		ext::future<std::result_of_t<std::decay_t<Functor>(ext::future<Type>)>>
	{
		typedef std::result_of_t<std::decay_t<Functor>(ext::future<Type>)> return_type;
		ext::intrusive_ptr<shared_state_basic> state;

		auto wrapped = [continuation = std::forward<Functor>(continuation),
		                self_ptr = ext::intrusive_ptr<self_type>(this)]() -> return_type
		{
			return continuation(ext::future<Type>(std::move(self_ptr)));
		};

		if (is_deffered())
		{
			typedef deferred_continuation_task<decltype(wrapped), return_type> ct_type;
			state = make_intrusive<ct_type>(std::move(wrapped));
		}
		else
		{
			typedef continuation_task<decltype(wrapped), return_type> ct_type;
			state = make_intrusive<ct_type>(std::move(wrapped));
		}

		add_continuation(state.get());
		return {state};
	}

	template <class Type, class Functor>
	auto shared_state_basic::add_shared_continuation(Functor && continuation) ->
		ext::future<std::result_of_t<std::decay_t<Functor>(ext::shared_future<Type>)>>
	{
		typedef std::result_of_t<std::decay_t<Functor>(ext::shared_future<Type>)> return_type;
		ext::intrusive_ptr<shared_state_basic> state;

		auto wrapped = [continuation = std::forward<Functor>(continuation),
		                self_ptr = ext::intrusive_ptr<self_type>(this)]() -> return_type
		{
			return continuation(ext::shared_future<Type>(std::move(self_ptr)));
		};

		if (is_deffered())
		{
			typedef deferred_continuation_task<decltype(wrapped), return_type> ct_type;
			state = make_intrusive<ct_type>(std::move(wrapped));
		}
		else
		{
			typedef continuation_task<decltype(wrapped), return_type> ct_type;
			state = make_intrusive<ct_type>(std::move(wrapped));
		}

		add_continuation(state.get());
		return {state};
	}

	/************************************************************************/
	/*          shared_state<Type>      method implementation               */
	/************************************************************************/
	template <class Type>
	void * shared_state<Type>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return &m_val;
			case future_state::exception:     std::rethrow_exception(m_exptr);
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	template <class Type>
	void shared_state<Type>::set_value(const value_type & val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		// construct in uninitialized memory
		new (&m_val) value_type(val);
		set_future_ready();
	}

	template <class Type>
	void shared_state<Type>::set_value(value_type && val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		// construct in uninitialized memory
		new (&m_val) value_type(std::move(val));
		set_future_ready();
	}

	template <class Type>
	void shared_state<Type>::set_exception(std::exception_ptr ex)
	{
		if (not satisfy_check_promise(future_state::exception)) return;

		// construct in uninitialized memory
		new (&m_exptr) std::exception_ptr(std::move(ex));
		set_future_ready();
	}

	template <class Type>
	shared_state<Type>::shared_state() noexcept {};

	template <class Type>
	shared_state<Type>::~shared_state() noexcept
	{
		switch (status())
		{
			case future_state::value:         m_val.~value_type();      break;
			case future_state::exception:     m_exptr.~exception_ptr(); break;
			case future_state::cancellation:
			case future_state::abandonned:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				break;
		}
	}

	/************************************************************************/
	/*          shared_state<Type &>      method implementation             */
	/************************************************************************/
	template <class Type>
	void * shared_state<Type &>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return ext::unconst(m_val);
			case future_state::exception:     std::rethrow_exception(m_exptr);
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	template <class Type>
	void shared_state<Type &>::set_value(value_type & val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		m_val = &val;
		set_future_ready();
	}

	template <class Type>
	void shared_state<Type &>::set_exception(std::exception_ptr ex)
	{
		if (not satisfy_check_promise(future_state::exception)) return;

		// construct in uninitialized memory
		new (&m_exptr) std::exception_ptr(std::move(ex));
		set_future_ready();
	}

	template <class Type>
	shared_state<Type &>::shared_state() noexcept {};

	template <class Type>
	shared_state<Type &>::~shared_state() noexcept
	{
		switch (status())
		{
			case future_state::value:         break;
			case future_state::exception:     m_exptr.~exception_ptr(); break;

			case future_state::cancellation:
			case future_state::abandonned:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				break;
		}
	}

	/************************************************************************/
	/*          shared_state<void>      method implementation               */
	/************************************************************************/
	inline void * shared_state<void>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return nullptr;
			case future_state::exception:     std::rethrow_exception(m_exptr);
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	inline void shared_state<void>::set_value(void)
	{
		if (not satisfy_check_promise(future_state::value)) return;
		set_future_ready();
	}

	inline void shared_state<void>::set_exception(std::exception_ptr ex)
	{
		if (not satisfy_check_promise(future_state::exception)) return;

		m_exptr = std::move(ex);
		set_future_ready();
	}

	inline shared_state<void>::shared_state() noexcept {};

	inline shared_state<void>::~shared_state() noexcept
	{
		switch (status())
		{
			case future_state::value:         break;
			case future_state::exception:     m_exptr.~exception_ptr(); break;

			case future_state::cancellation:
			case future_state::abandonned:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				break;
		}
	}



	/************************************************************************/
	/*      shared_state_unexceptional<Type>      method implementation     */
	/************************************************************************/
	template <class Type>
	void * shared_state_unexceptional<Type>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return &m_val;
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::exception:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	template <class Type>
	void shared_state_unexceptional<Type>::set_value(const value_type & val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		// construct in uninitialized memory
		new (&m_val) value_type(val);
		set_future_ready();
	}

	template <class Type>
	void shared_state_unexceptional<Type>::set_value(value_type && val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		// construct in uninitialized memory
		new (&m_val) value_type(std::move(val));
		set_future_ready();
	}

	template <class Type>
	shared_state_unexceptional<Type>::shared_state_unexceptional() noexcept {};

	template <class Type>
	shared_state_unexceptional<Type>::~shared_state_unexceptional() noexcept
	{
		switch (status())
		{
			case future_state::value:         m_val.~value_type(); break;
			case future_state::exception:
			case future_state::cancellation:
			case future_state::abandonned:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				break;
		}
	}

	/************************************************************************/
	/*      shared_state_unexceptional<Type &>    method implementation     */
	/************************************************************************/
	template <class Type>
	void * shared_state_unexceptional<Type &>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return ext::unconst(m_val);
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::exception:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	template <class Type>
	void shared_state_unexceptional<Type &>::set_value(value_type & val)
	{
		if (not satisfy_check_promise(future_state::value)) return;

		m_val = &val;
		set_future_ready();
	}

	/************************************************************************/
	/*        shared_state_unexceptional<void>    method implementation     */
	/************************************************************************/
	inline void * shared_state_unexceptional<void>::get_ptr()
	{
		this->wait();
		// wait checks m_fstnext for ready with std::memory_order_relaxed
		// to see m_val, we must synchromize with release operation in set_* functions
		std::atomic_thread_fence(std::memory_order_acquire);
		switch (status())
		{
			case future_state::value:         return nullptr;
			case future_state::cancellation:  throw future_error(make_error_code(future_errc::cancelled));
			case future_state::abandonned:    throw future_error(make_error_code(future_errc::broken_promise));

			case future_state::exception:
			case future_state::unsatisfied:
			case future_state::deffered:
			default:
				EXT_UNREACHABLE();
		}
	}

	inline void shared_state_unexceptional<void>::set_value(void)
	{
		if (not satisfy_check_promise(future_state::value)) return;
		set_future_ready();
	}
	
	/************************************************************************/
	/*              tasks implementation                                    */
	/************************************************************************/
	template <class Functor, class Ret, class ... Args>
	inline void shared_state_execute(shared_state<Ret> & sh, Functor & functor, Args && ... args)
	{
		sh.set_value(ext::invoke(functor, std::move(args)...));
	}

	template <class Functor, class ... Args>
	inline void shared_state_execute(shared_state<void> & sh, Functor & functor, Args && ... args)
	{
		ext::invoke(functor, std::forward(args)...);
		sh.set_value();
	}

	template <class Functor, class Ret, class ... Args>
	inline void shared_state_execute(shared_state_unexceptional<Ret> & sh, Functor & functor, Args && ... args)
	{
		sh.set_value(ext::invoke(functor, std::move(args)...));
	}

	template <class Functor, class ... Args>
	inline void shared_state_execute(shared_state_unexceptional<void> & sh, Functor & functor, Args && ... args)
	{
		ext::invoke(functor, std::forward(args)...);
		sh.set_value();
	}

	template <class Functor, class Ret, class ... Args>
	void packaged_task_impl<Functor, Ret(Args...)>::execute(Args ... args) noexcept
	{
		if (not this->mark_uncancellable())
			return;

		try
		{
			shared_state_execute(*this, m_functor, std::move(args)...);
		}
		catch (...)
		{
			this->set_exception(std::current_exception());
		}
	}

	template <class Functor, class Ret, class ... Args>
	auto packaged_task_impl<Functor, Ret(Args...)>::reset() -> self_type *
	{
		this->release_promise();
		return new self_type(std::move(m_functor));
	}

	template <class Functor, class Ret>
	void deferred_task_impl<Functor, Ret()>::wait() const
	{
		if (not ext::unconst(this)->mark_uncancellable())
			return base_type::wait();

		try
		{
			shared_state_execute(*ext::unconst(this), ext::unconst(this)->m_functor);
		}
		catch (...)
		{
			ext::unconst(this)->set_exception(std::current_exception());
		}
	}

	template <class Functor, class Type>
	void continuation_task<Functor, Type>::set_future_ready() noexcept
	{
		auto fstate = signal_future(m_task_next);
		run_continuations(fstate);
		 
		//auto next = base_type::m_fstnext.load(std::memory_order_acquire);
		//run_continuations(next);
	}

	template <class Functor, class Type>
	void continuation_task<Functor, Type>::continuate() noexcept
	{
		this->execute();
	}

	template <class Functor, class Type>
	void deferred_continuation_task<Functor, Type>::wait() const
	{
		if (not ext::unconst(this)->mark_uncancellable())
			return base_type::wait();

		try
		{
			shared_state_execute(*ext::unconst(this), ext::unconst(this)->m_functor);
		}
		catch (...)
		{
			ext::unconst(this)->set_exception(std::current_exception());
		}
	}

	template <class Type>
	void when_all_task<Type>::notify_satisfied(std::size_t) noexcept
	{
		auto prev_count = m_count.fetch_sub(1, std::memory_order_relaxed);
		if (prev_count == 1 && this->mark_uncancellable())
		{
			// m_val already set in constructor
			this->satisfy_promise(future_state::value);
			this->set_future_ready();
		}
	}

	template <class Type>
	void when_any_task<Type>::notify_satisfied(std::size_t index) noexcept
	{
		if (this->mark_uncancellable())
		{
			this->m_val.index = index;
			this->satisfy_promise(future_state::value);
			this->set_future_ready();
		}
	}

	/************************************************************************/
	/*                    front-end classes                                 */
	/************************************************************************/
	template <class Type>
	class future
	{
	public:
		typedef Type value_type;
		typedef ext::intrusive_ptr<shared_state_basic> intrusive_ptr;

	private:
		intrusive_ptr m_ptr;

	public:
		// low-level helpers
		const intrusive_ptr & handle() const noexcept  { return m_ptr; }
		      intrusive_ptr   release() const noexcept { return std::move(m_ptr); }

	public:
		bool is_pending()    const noexcept { return m_ptr ? m_ptr->is_pending()    : false; }
		bool is_ready()      const noexcept { return m_ptr ? m_ptr->is_ready()      : false; }
		bool is_abandoned()  const noexcept { return m_ptr ? m_ptr->is_abandoned()  : false; }
		bool is_cancelled()  const noexcept { return m_ptr ? m_ptr->is_cancelled()  : false; }
		bool is_deffered()   const noexcept { return m_ptr ? m_ptr->is_deffered()   : false; }
		bool has_value()     const noexcept { return m_ptr ? m_ptr->has_value()     : false; }
		bool has_exception() const noexcept { return m_ptr ? m_ptr->has_exception() : false; }

		bool valid() const { return static_cast<bool>(m_ptr); }
		bool cancel() { assert(valid()); return m_ptr->cancel(); }
		value_type get() { assert(valid()); auto ptr = std::move(m_ptr); return ptr->template get<value_type>(); }


		ext::shared_future<Type> share() { assert(valid()); return std::move(m_ptr); }

		void wait() const { assert(valid()); return m_ptr->wait(); }
		future_status wait_for(std::chrono::steady_clock::duration timeout_duration)  const { assert(valid()); return m_ptr->wait_for(timeout_duration); }
		future_status wait_until(std::chrono::steady_clock::time_point timeout_point) const { assert(valid()); return m_ptr->wait_until(timeout_point); }

		template <class Functor>
		auto then(Functor && continuation) ->
			ext::future<std::result_of_t<std::decay_t<Functor>(ext::future<value_type>)>>
		{
			assert(valid());
			auto self = std::move(*this);
			return self.m_ptr->template add_unique_continuation<value_type>(std::forward<Functor>(continuation));
		}

	public:
		future() = default;
		future(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {}

		future(future &&) = default;
		future(const future &) = delete;

		future & operator =(future &&) = default;
		future & operator =(const future &) = delete;
	};


	template <class Type>
	class shared_future
	{
	public:
		typedef Type value_type;
		typedef ext::intrusive_ptr<shared_state_basic> intrusive_ptr;

	private:
		intrusive_ptr m_ptr;

	public:
		// low-level helpers
		const intrusive_ptr & handle() const noexcept  { return m_ptr; }
		      intrusive_ptr   release() const noexcept { return std::move(m_ptr); }

	public:
		bool is_pending()    const noexcept { return m_ptr ? m_ptr->is_pending()    : false; }
		bool is_ready()      const noexcept { return m_ptr ? m_ptr->is_ready()      : false; }
		bool is_abandoned()  const noexcept { return m_ptr ? m_ptr->is_abandoned()  : false; }
		bool is_cancelled()  const noexcept { return m_ptr ? m_ptr->is_cancelled()  : false; }
		bool is_deffered()   const noexcept { return m_ptr ? m_ptr->is_deffered()   : false; }
		bool has_value()     const noexcept { return m_ptr ? m_ptr->has_value()     : false; }
		bool has_exception() const noexcept { return m_ptr ? m_ptr->has_exception() : false; }

		bool valid() const { return static_cast<bool>(m_ptr); }
		bool cancel() { assert(valid()); return m_ptr->cancel(); }
		value_type get() { assert(valid()); return m_ptr->template get<value_type>(); }

		void wait() const { assert(valid()); return m_ptr->wait(); }
		future_status wait_for(std::chrono::steady_clock::duration timeout_duration)  const { assert(valid()); return m_ptr->wait_for(timeout_duration); }
		future_status wait_until(std::chrono::steady_clock::time_point timeout_point) const { assert(valid()); return m_ptr->wait_until(timeout_point); }

		template <class Functor>
		auto then(Functor && continuation) ->
			ext::future<std::result_of_t<std::decay_t<Functor>(ext::shared_future<value_type>)>>
		{
			assert(valid());
			return m_ptr->template add_shared_continuation<value_type>(std::forward<Functor>(continuation));
		}

	public:
		shared_future() = default;
		// shared_future can only be constructed by moving future
		shared_future(const future<Type> & f) = delete;
		shared_future(future<Type> && f) : shared_future(f.share()) {}
		shared_future(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {}

		shared_future(shared_future &&) = default;
		shared_future(const shared_future &) = default;

		shared_future & operator =(shared_future &&) = default;
		shared_future & operator =(const shared_future &) = default;
	};

	/************************************************************************/
	/*                   promise<Type>                                      */
	/************************************************************************/
	template <class Type>
	class promise
	{
	public:
		typedef Type value_type;
		typedef ext::intrusive_ptr<ext::shared_state<value_type>> intrusive_ptr;

	private:
		intrusive_ptr m_ptr;

	private:
		void check_state();

	public:
		ext::future<value_type> get_future();

		bool cancel();
		void set_value(const value_type & val);
		void set_value(value_type && val);
		void set_exception(std::exception_ptr ex);

	public:
		promise() noexcept : m_ptr(ext::make_intrusive<ext::shared_state<value_type>>()) {}
		promise(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {}
		~promise() noexcept { if (m_ptr) m_ptr->release_promise(); }

		promise(promise &&) = default;
		promise(const promise &) = delete;

		promise & operator =(promise &&) = default;
		promise & operator =(const promise &) = delete;

		void swap(promise & other) noexcept { std::swap(m_ptr, other.m_ptr); }
	};

	template <class Type>
	inline void promise<Type>::check_state()
	{
		if (not m_ptr)
			throw future_error(make_error_code(future_errc::no_state));
	}

	template <class Type>
	inline auto promise<Type>::get_future() -> ext::future<value_type>
	{
		check_state();
		m_ptr->mark_retrived();
		return {m_ptr};
	}

	template <class Type>
	inline bool promise<Type>::cancel()
	{
		check_state();
		return m_ptr->cancel();
	}

	template <class Type>
	inline void promise<Type>::set_value(value_type && val)
	{
		check_state();
		m_ptr->set_value(std::move(val));
	}

	template <class Type>
	inline void promise<Type>::set_value(const value_type & val)
	{
		check_state();
		m_ptr->set_value(val);
	}

	template <class Type>
	inline void promise<Type>::set_exception(std::exception_ptr ex)
	{
		check_state();
		m_ptr->set_exception(ex);
	}



	/// promise<Type &>
	template <class Type>
	class promise<Type &>
	{
	public:
		typedef Type value_type;
		typedef ext::intrusive_ptr<ext::shared_state<value_type &>> intrusive_ptr;

	private:
		intrusive_ptr m_ptr;

	private:
		void check_state();

	public:
		ext::future<value_type &> get_future();

		bool cancel();
		void set_value(value_type & val);
		void set_exception(std::exception_ptr ex);

	public:
		promise() : m_ptr(ext::make_intrusive<ext::shared_state<value_type &>>()) {}
		promise(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {}
		~promise() noexcept { if (m_ptr) m_ptr->release_promise(); }

		promise(promise &&) = default;
		promise(const promise &) = delete;

		promise & operator =(promise &&) = default;
		promise & operator =(const promise &) = delete;

		void swap(promise & other) noexcept { std::swap(m_ptr, other.m_ptr); }
	};

	template <class Type>
	inline void promise<Type &>::check_state()
	{
		if (not m_ptr)
			throw future_error(make_error_code(future_errc::no_state));
	}

	template <class Type>
	inline auto promise<Type &>::get_future() -> ext::future<value_type &>
	{
		check_state();
		m_ptr->mark_retrived();
		return {m_ptr};
	}

	template <class Type>
	inline bool promise<Type &>::cancel()
	{
		check_state();
		return m_ptr->cancel();
	}

	template <class Type>
	inline void promise<Type &>::set_value(value_type & val)
	{
		check_state();
		m_ptr->set_value(std::move(val));
	}

	template <class Type>
	inline void promise<Type &>::set_exception(std::exception_ptr ex)
	{
		check_state();
		m_ptr->set_exception(ex);
	}

	/// promise<void>
	template <>
	class promise<void>
	{
	public:
		typedef void value_type;
		typedef ext::intrusive_ptr<ext::shared_state<value_type>> intrusive_ptr;

	private:
		intrusive_ptr m_ptr;

	private:
		void check_state();

	public:
		ext::future<value_type> get_future();

		bool cancel();
		void set_value(void);
		void set_exception(std::exception_ptr ex);

	public:
		promise() noexcept : m_ptr(ext::make_intrusive<ext::shared_state<value_type>>()) {}
		promise(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {}
		~promise() noexcept { if (m_ptr) m_ptr->release_promise(); }

		promise(promise &&) = default;
		promise(const promise &) = delete;

		promise & operator =(promise &&) = default;
		promise & operator =(const promise &) = delete;

		void swap(promise & other) noexcept { std::swap(m_ptr, other.m_ptr); }
	};

	inline void promise<void>::check_state()
	{
		if (not m_ptr)
			throw future_error(make_error_code(future_errc::no_state));
	}

	inline auto promise<void>::get_future() -> ext::future<value_type>
	{
		check_state();
		m_ptr->mark_retrived();
		return {m_ptr};
	}

	inline bool promise<void>::cancel()
	{
		check_state();
		return m_ptr->cancel();
	}

	inline void promise<void>::set_value(void)
	{
		check_state();
		m_ptr->set_value();
	}

	inline void promise<void>::set_exception(std::exception_ptr ex)
	{
		check_state();
		m_ptr->set_exception(ex);
	}

	/************************************************************************/
	/*             packaged_task<RetType(Args...)>                          */
	/************************************************************************/
	template <class RetType, class ... Args>
	class packaged_task<RetType(Args...)>
	{
		typedef ext::intrusive_ptr<packaged_task_base<RetType(Args...)>> intrusive_ptr;
		template <class Functor> using impl_type = packaged_task_impl<Functor, RetType(Args...)>;

	private:
		intrusive_ptr m_ptr;

	private:
		void check_state();

	public:
		bool valid() const noexcept { return m_ptr ? m_ptr->valid() : false; }
		ext::future<RetType> get_future();

		void reset();
		void operator()(Args ... args);

	public:
		packaged_task() = default;
		packaged_task(intrusive_ptr ptr) noexcept : m_ptr(std::move(ptr)) {};
		~packaged_task() noexcept { if (m_ptr) m_ptr->release_promise(); }

		template <class Functor>
		packaged_task(Functor && functor) :
			m_ptr(ext::make_intrusive<impl_type<std::decay_t<Functor>>>(std::forward<Functor>(functor))) {}

		packaged_task(packaged_task &&) = default;
		packaged_task(const packaged_task &) = delete;

		packaged_task & operator =(packaged_task &&) = default;
		packaged_task & operator =(const packaged_task &) = delete;

		void swap(packaged_task & other) noexcept { std::swap(m_ptr, other.m_ptr); }
	};

	template <class RetType, class ... Args>
	inline void packaged_task<RetType(Args ...)>::check_state()
	{
		if (not m_ptr)
			throw future_error(make_error_code(future_errc::no_state));
	}
	
	template <class RetType, class ... Args>
	inline auto packaged_task<RetType(Args ...)>::get_future() -> ext::future<RetType>
	{
		check_state();
		m_ptr->mark_retrived();
		return {m_ptr};
	}

	template <class RetType, class ... Args>
	inline void packaged_task<RetType(Args ...)>::reset()
	{
		check_state();
		*this = intrusive_ptr(m_ptr->reset(), ext::noaddref);
	}

	template <class RetType, class ... Args>
	inline void packaged_task<RetType(Args ...)>::operator()(Args ... args)
	{
		check_state();
		m_ptr->execute(std::move(args)...);
	}


	template <class Type>
	ext::future<std::decay_t<Type>> make_ready_future(Type && val)
	{
		auto ptr = ext::make_intrusive<ext::shared_state_unexceptional<std::decay_t<Type>>>();
		ptr->set_value(std::forward<Type>(val));
		return {ptr};
	}
	
	inline ext::future<void> make_ready_future()
	{
		auto ptr = ext::make_intrusive<ext::shared_state_unexceptional<void>>();
		ptr->set_value();
		return {ptr};
	}

	template<class Function, class... Args>
	auto async(ext::launch policy, Function && func, Args && ... args) ->
		future<std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)>>
	{
		typedef std::result_of_t<std::decay_t<Function>(std::decay_t<Args>...)> result_type;

		auto closure = [func = std::forward<Function>(func),
		                args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> result_type
		{
			return ext::apply(func, std::move(args_tuple));
		};

		if (static_cast<unsigned>(policy) & static_cast<unsigned>(ext::launch::async))
		{
			ext::packaged_task<result_type()> pt {std::move(closure)};
			auto result = pt.get_future();

			std::thread thx {std::move(pt)};
			thx.detach();

			return result;
		}
		else //if (static_cast<unsigned>(policy) & static_cast<unsigned>(ext::launch::deferred))
		{
			auto pt = ext::make_intrusive<deferred_task_impl<decltype(closure), result_type()>>(std::move(closure));
			return ext::future<result_type>(std::move(pt));
		}
	}


	template <class InputIterator>
	auto when_any(InputIterator first, InputIterator last) ->
		std::enable_if_t<
			is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
			ext::future<when_any_result<std::vector<typename std::iterator_traits<InputIterator>::value_type>>>
		>
	{
		typedef typename std::iterator_traits<InputIterator>::value_type value_type;
		typedef when_any_result<std::vector<value_type>> result_type;
		typedef when_any_task<result_type> state_type;

		result_type result;
		result.index = SIZE_MAX;
		auto & futures = result.futures;
		ext::try_reserve(futures, first, last);

		for (; first != last; ++first)
			futures.push_back(*first);

		if (futures.empty())
			return make_ready_future<result_type>(std::move(result));

		std::size_t idx = 0;
		auto state = ext::make_intrusive<state_type>(std::move(result));
		for (const auto & f : state->m_val.futures)
		{
			if (f.is_deffered())
			{
				state->notify_satisfied(idx);
				break;
			}

			auto cont = ext::make_intrusive<when_any_task_continuation>(state, idx++);
			if (not f.handle()->add_continuation(cont.get())) 
				break; // executed immediately, no sense to continue
		}

		return {state};
	}

	template <class ... Futures>
	auto when_any(Futures && ... futures) ->
		std::enable_if_t<
			is_future_types<std::decay_t<Futures>...>::value,
			ext::future<when_any_result<std::tuple<std::decay_t<Futures>...>>>
		>
	{
		typedef std::tuple<std::decay_t<Futures>...> tuple_type;
		typedef when_any_result<tuple_type> result_type;
		typedef when_any_task<result_type> state_type;

		std::initializer_list<ext::shared_state_basic *> handles = {futures.handle().get()...};
		result_type result {SIZE_MAX, tuple_type {std::forward<Futures>(futures)...}};

		std::size_t idx = 0;
		auto state = ext::make_intrusive<state_type>(std::move(result));
		for (auto * handle : handles)
		{
			if (handle->is_deffered())
			{
				state->notify_satisfied(idx);
				break;
			}

			auto cont = ext::make_intrusive<when_any_task_continuation>(state, idx++);
			if (not handle->add_continuation(cont.get()))
				break; // executed immediately, no sense to continue
		}

		return {state};
	}


	template <class InputIterator>
	auto when_all(InputIterator first, InputIterator last) ->
		std::enable_if_t<
			is_future_type<typename std::iterator_traits<InputIterator>::value_type>::value,
			ext::future<std::vector<typename std::iterator_traits<InputIterator>::value_type>>
		>
	{
		typedef typename std::iterator_traits<InputIterator>::value_type value_type;
		typedef std::vector<value_type> result_type;
		typedef when_all_task<result_type> state_type;

		result_type futures;
		ext::try_reserve(futures, first, last);

		for (; first != last; ++first)
			futures.push_back(*first);

		if (futures.empty())
			return make_ready_future<result_type>(std::move(futures));

		auto state = ext::make_intrusive<state_type>(std::move(futures), futures.size());
		for (const auto & f : state->m_val)
		{
			if (f.is_deffered())
				state->notify_satisfied(0);
			else
			{
				auto cont = ext::make_intrusive<when_all_task_continuation>(state);
				f.handle()->add_continuation(cont.get());
			}
		}

		return {state};
	}

	template <class ... Futures>
	auto when_all(Futures && ... futures) ->
		std::enable_if_t<
			is_future_types<std::decay_t<Futures>...>::value,
			ext::future<std::tuple<std::decay_t<Futures>...>>
		>
	{
		typedef std::tuple<std::decay_t<Futures>...> result_type;
		typedef when_all_task<result_type> state_type;

		std::initializer_list<ext::shared_state_basic *> handles = {futures.handle().get()...};
		result_type ftuple {std::forward<Futures>(futures)...};

		auto state = ext::make_intrusive<state_type>(std::move(ftuple), sizeof...(futures));
		for (auto * handle : handles)
		{
			if (handle->is_deffered())
				state->notify_satisfied(0);
			else
			{
				auto cont = ext::make_intrusive<when_all_task_continuation>(state);
				handle->add_continuation(cont.get());
			}
		}

		return {state};
	}


	/************************************************************************/
	/*                   swap non member functions                          */
	/************************************************************************/
	template <class Type>
	inline void swap(future<Type> & f1, future<Type> & f2) noexcept
	{
		f1.swap(f2);
	}

	template <class Type>
	inline void swap(shared_future<Type> & f1, shared_future<Type> & f2) noexcept
	{
		f1.swap(f2);
	}

	template <class Type>
	inline void swap(promise<Type> & f1, promise<Type> & f2) noexcept
	{
		f1.swap(f2);
	}

	template <class Type>
	inline void swap(packaged_task<Type> & f1, packaged_task<Type> & f2) noexcept
	{
		f1.swap(f2);
	}
}

namespace std
{
	template <>
	struct is_error_code_enum<ext::future_errc>
		: std::true_type {};
}
