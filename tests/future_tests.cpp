#include <future>
#include <ext/future.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>
#include <boost/test/unit_test.hpp>

struct future_fixture
{
	future_fixture()  { ext::init_future_library(); }
	~future_fixture() { ext::free_future_library(); }
};

BOOST_FIXTURE_TEST_SUITE(future_tests, future_fixture)

BOOST_AUTO_TEST_CASE(future_simple_test)
{
	using namespace std;
	
	ext::promise<void> pi;
	ext::shared_future<void> fi = pi.get_future();
	unsigned count = 0;

	auto fs = fi.then([&count](auto && f) { ++count; });
	auto ff = fi.then([&count](auto && f) { ++count; });

	BOOST_CHECK(fi.is_pending());
	BOOST_CHECK(fs.is_pending());

	pi.set_value();

	BOOST_CHECK(fi.has_value());
	BOOST_CHECK(fs.has_value());

	BOOST_CHECK(count == 2);

	pi = ext::promise<void>();
	pi.set_exception(std::make_exception_ptr(std::exception()));

	fi = pi.get_future();

	BOOST_CHECK(fi.has_exception());
}

BOOST_AUTO_TEST_CASE(future_cancellation_tests)
{
	{
		ext::promise<void> pi;
		ext::shared_future<void> fi = pi.get_future();
		unsigned count = 0;

		auto fs = fi.then([&count](auto && f) { ++count; });
		auto ff = fi.then([&count](auto && f) { ++count; });

		bool cancelled = fs.cancel();
		pi.set_value();

		BOOST_CHECK_EQUAL(cancelled, true);
		BOOST_CHECK_EQUAL(count, 1);
		BOOST_CHECK(fs.is_cancelled());

		BOOST_CHECK_EXCEPTION(
			fs.get(), ext::future_error,
			[](auto & ex) { return ex.code() == ext::future_errc::cancelled; }
		);
	}

	{
		unsigned uc1 = 0;
		unsigned uc2 = 0;

		ext::shared_future<int> fi = ext::async(ext::launch::deferred, [] { return 12; });
		ext::shared_future<void> f11 = fi.then([&uc1](auto && f)  { if (!f.is_cancelled()) ++uc1; });
		ext::shared_future<void> f12 = f11.then([&uc1](auto && f) { if (!f.is_cancelled()) ++uc1; });

		ext::shared_future<void> f21 = fi.then([&uc2](auto && f)  { if (!f.is_cancelled()) ++uc2; });
		ext::shared_future<void> f22 = f21.then([&uc2](auto && f) { if (!f.is_cancelled()) ++uc2; });

		f11.cancel();
		f22.cancel();
		
		// currently continuations on deferred run immediately
		auto res = fi.get();
		BOOST_CHECK_EQUAL(res, 12);
		BOOST_CHECK_EQUAL(uc1, 2);
		BOOST_CHECK_EQUAL(uc2, 2);
	}
}

BOOST_AUTO_TEST_CASE(future_broken_tests)
{
	ext::future<void> fi;

	{
		ext::promise<void> pi;
		fi = pi.get_future();
	}
	
	BOOST_CHECK(fi.is_abandoned());
	BOOST_CHECK_EXCEPTION(
		fi.get(), ext::future_error,
		[](auto & ex) { return ex.code() == ext::future_errc::broken_promise; }
	);
}

BOOST_AUTO_TEST_CASE(future_when_all_tests)
{
	using namespace std;
	{
		auto f1 = ext::async(ext::launch::async, [] { return 12; });
		auto f2 = ext::async(ext::launch::async, [] { return "12"s; });

		auto fr = ext::when_all(std::move(f1), std::move(f2));
		auto wait_res = fr.wait_for(5s);
		BOOST_REQUIRE(wait_res == ext::future_status::ready);
		
		auto tf = fr.get();
		auto i = std::get<0>(tf).get();
		auto s = std::get<1>(tf).get();

		BOOST_CHECK_EQUAL(i, 12);
		BOOST_CHECK_EQUAL(s, "12");
	}

	{
		ext::shared_future<int> farr[] =
		{
			ext::async(ext::launch::deferred, [] { return 12; }),
			ext::async(ext::launch::deferred, [] { return 24; }),
		};

		auto fres = ext::when_all(std::begin(farr), std::end(farr));
		BOOST_REQUIRE(fres.is_ready());
		
		auto res = fres.get();
		auto v1 = res[0].get();
		auto v2 = res[1].get();

		BOOST_CHECK_EQUAL(v1, 12);
		BOOST_CHECK_EQUAL(v2, 24);
	}
}

BOOST_AUTO_TEST_CASE(future_when_any_tests)
{
	using namespace std;

	{
		ext::promise<string> ps;

		auto fi = ext::async(ext::launch::deferred, [] { return 12; });
		auto fs = ps.get_future();

		auto fres = ext::when_any(std::move(fi), std::move(fs));
		BOOST_REQUIRE(fres.is_ready());

		auto res = fres.get();
		BOOST_CHECK_EQUAL(res.index, 0);

		auto ready = ext::visit(res.futures, res.index, [](auto && f) { return f.is_ready(); });
		BOOST_CHECK_EQUAL(ready, true);
	}

	{
		ext::promise<int> ps;
		ext::shared_future<int> farr[] =
		{
			ext::async(ext::launch::deferred, [] { return 12; }),
			ps.get_future()
		};

		auto fres = ext::when_any(std::begin(farr), std::end(farr));
		BOOST_REQUIRE(fres.is_ready());

		auto res = fres.get();
		BOOST_CHECK_EQUAL(res.index, 0);

		auto ready = res.futures[res.index].is_ready();
		BOOST_CHECK(ready);
	}
}

BOOST_AUTO_TEST_CASE(future_deferred_tests)
{
	auto f = ext::async(ext::launch::deferred, [] { return 12u; });
	auto fc = f.then([](auto f) { return f.get() + 12u; });

	//BOOST_CHECK(fc.is_deferred());
	fc.wait();
	BOOST_CHECK(fc.is_ready());
	BOOST_CHECK_EQUAL(fc.get(), 24);
	BOOST_CHECK(not fc.valid());

	auto f1 = ext::async(ext::launch::deferred, [] { return 12u; });
	auto f2 = ext::async(ext::launch::deferred, [] { return 12u; });
	auto fall = ext::when_all(std::move(f1), std::move(f2));

	BOOST_CHECK(fall.is_ready());
	std::tie(f1, f2) = fall.get();
	BOOST_CHECK_EQUAL(f1.get() + f2.get(), 24);
}

BOOST_AUTO_TEST_CASE(future_deferred_unwrap_tests)
{
	// async => deferred
	{
		auto f1 = ext::async(ext::launch::async, [] { return 12; });
		auto f2 = f1.then([](auto f)
		{
			auto val = f.get();
			return ext::async(ext::launch::deferred, [val] { return val * 2 + 3; });
		});

		auto fall = ext::when_all(f2.unwrap());
		ext::future<int> fi = std::get<0>(fall.get());
		BOOST_CHECK_EQUAL(fi.get(), 12 * 2 + 3);
	}
	
	// deferred => async
	{
		auto f1 = ext::async(ext::launch::deferred, [] { return 12; });
		auto f2 = f1.then([](auto f)
		{
			auto val = f.get();
			return ext::async(ext::launch::async, [val] { return val * 2 + 3; });
		});

		auto fall = ext::when_all(f2.unwrap());
		ext::future<int> fi = std::get<0>(fall.get());
		BOOST_CHECK_EQUAL(fi.get(), 12 * 2 + 3);
	}
}

// functor who return different types and values depending on how called:
// * via const lvalue ref (const &)
// * via       lvalue ref (      &)
// * via       rvalue ref (     &&)
struct tricky_functor
{
	   int operator()() const &  noexcept { return 1;   } // if called as const object
	  long operator()()       &  noexcept { return 2;   } // if called as non const object
	double operator()()       && noexcept { return 3.0; } // if called as tmp object

	// for continuations
	template <class Arg> 	int operator()(Arg && arg) const &  noexcept { return 1;   } // if called as const object
	template <class Arg>   long operator()(Arg && arg)       &  noexcept { return 2;   } // if called as non const object
	template <class Arg> double operator()(Arg && arg)       && noexcept { return 3.0; } // if called as tmp object
};

BOOST_AUTO_TEST_CASE(std_future_result_type_tests)
{
	tricky_functor func;
	BOOST_CHECK_EQUAL(func(), 2);
	BOOST_CHECK_EQUAL(std::as_const(func)(), 1);
	BOOST_CHECK_EQUAL(tricky_functor()(), 3);

	// No matter how functor was passed into packaged_task, by moving or just a copy,
	// because of reset method/functionality packaged_task(functors must be copied/moved to a new task) functor is always called like functor(...).
	// Note: return value_type is passed explicitly to packaged_task.
	std::packaged_task<double()> task(tricky_functor{});
	auto ftask = task.get_future(); task();
	BOOST_CHECK_EQUAL(ftask.get(), 2);

	task = std::packaged_task<double()>(func);
	ftask = task.get_future(); task();
	BOOST_CHECK_EQUAL(ftask.get(), 2);

	std::shared_future<double> fasync, fdeferred;

	// Internally func will be copied/moved to async thread, and there will be called exactly once.
	// As result it can and should be called like std::move(functor)(...)
	fasync = std::async(std::launch::async, func);
	BOOST_CHECK_EQUAL(fasync.get(), 3);
	// even more reasons to call it like std::move(functor)(...)
	fasync = std::async(std::launch::async, tricky_functor());
	BOOST_CHECK_EQUAL(fasync.get(), 3);

	// launch::deferred should behave like launch::async
	fdeferred = std::async(std::launch::deferred, func);
	BOOST_CHECK_EQUAL(fdeferred.get(), 3);
	fdeferred = std::async(std::launch::deferred, tricky_functor());
	BOOST_CHECK_EQUAL(fdeferred.get(), 3);
}

BOOST_AUTO_TEST_CASE(future_result_type_tests)
{
	tricky_functor func;
	BOOST_CHECK_EQUAL(func(), 2);
	BOOST_CHECK_EQUAL(ext::as_const(func)(), 1);
	BOOST_CHECK_EQUAL(tricky_functor()(), 3);

	// No matter how functor was passed into packaged_task, by moving or just a copy,
	// because of reset method/functionality packaged_task(functors must be copied/moved to a new task) functor is always called like functor(...).
	// Note: return value_type is passed explicitly to pcakged_task.
	ext::packaged_task<double()> task(tricky_functor{});
	auto ftask = task.get_future(); task();
	BOOST_CHECK_EQUAL(ftask.get(), 2);

	task = ext::packaged_task<double()>(func);
	ftask = task.get_future(); task();
	BOOST_CHECK_EQUAL(ftask.get(), 2);

	ext::shared_future<double> fasync, fdeferred;

	// Internally func will be copied/moved to async thread, and there will be called exactly once.
	// As result it can and should be called like std::move(functor)(...)
	fasync = ext::async(ext::launch::async, func);
	BOOST_CHECK_EQUAL(fasync.get(), 3);
	// even more reasons to call it like std::move(functor)(...)
	fasync = ext::async(ext::launch::async, tricky_functor());
	BOOST_CHECK_EQUAL(fasync.get(), 3);

	// launch::deferred should behave like launch::async
	fdeferred = ext::async(ext::launch::deferred, func);
	BOOST_CHECK_EQUAL(fdeferred.get(), 3);
	fdeferred = ext::async(ext::launch::deferred, tricky_functor());
	BOOST_CHECK_EQUAL(fdeferred.get(), 3);

	// same with continuations: they are copied/moved to a continuation and called exactly once -> should be called like std::move(functor)(...)
	BOOST_CHECK_EQUAL(fasync.then(func).get(), 3);
	BOOST_CHECK_EQUAL(fasync.then(tricky_functor()).get(), 3);
}

BOOST_AUTO_TEST_CASE(thread_pool_result_type_tests)
{
	tricky_functor func;

	ext::thread_pool pool;
	pool.set_nworkers(1);

	BOOST_CHECK_EQUAL(pool.submit(func).get(), 3);
	BOOST_CHECK_EQUAL(pool.submit(tricky_functor()).get(), 3);

	ext::shared_future<int> f = ext::make_ready_future(12);
	BOOST_CHECK_EQUAL(pool.submit(f, func).get(), 3);
	BOOST_CHECK_EQUAL(pool.submit(f, tricky_functor()).get(), 3);
}

BOOST_AUTO_TEST_CASE(threaded_scheduler_result_type_tests)
{
	tricky_functor func;
	ext::threaded_scheduler scheduler;

	using namespace std::chrono_literals;

	BOOST_CHECK_EQUAL(scheduler.submit(0s, func).get(), 3);
	BOOST_CHECK_EQUAL(scheduler.submit(0s, tricky_functor()).get(), 3);
}



BOOST_AUTO_TEST_CASE(thread_pool_tests)
{
	using namespace std::chrono_literals;
	ext::thread_pool pool(std::thread::hardware_concurrency());

	auto f1 = pool.submit([] { return 100; });
	auto f2 = ext::async(ext::launch::async, [] { std::this_thread::sleep_for(10ms); return 12;});
	auto f3 = pool.submit(std::move(f2), [](auto f) { return f.get() + 10; });

	int result = f1.get() + f3.get();
	BOOST_CHECK_EQUAL(result, 122);
}

BOOST_AUTO_TEST_SUITE_END()
