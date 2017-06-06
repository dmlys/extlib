#include <ext/future.hpp>
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

		BOOST_CHECK(cancelled == true);
		BOOST_CHECK(count == 1);
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
		
		auto res = fi.get();
		BOOST_CHECK(res == 12);
		BOOST_CHECK(uc1 == 0);
		BOOST_CHECK(uc2 == 1);
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

		BOOST_CHECK(i == 12);
		BOOST_CHECK(s == "12");
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

		BOOST_CHECK(v1 == 12);
		BOOST_CHECK(v2 == 24);
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
		BOOST_CHECK(res.index == 0);

		auto ready = ext::visit(res.futures, res.index, [](auto && f) { return f.is_ready(); });
		BOOST_CHECK(ready);
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
		BOOST_CHECK(res.index == 0);

		auto ready = res.futures[res.index].is_ready();
		BOOST_CHECK(ready);
	}
}

BOOST_AUTO_TEST_CASE(future_deferred_tests)
{
	auto f = ext::async(ext::launch::deferred, [] { return 12u; });
	auto fc = f.then([](auto f) { return f.get() + 12u; });

	BOOST_CHECK(fc.is_deferred());
	fc.wait();
	BOOST_CHECK(fc.is_ready());
	BOOST_CHECK(fc.get() == 24);
	BOOST_CHECK(not fc.valid());

	auto f1 = ext::async(ext::launch::deferred, [] { return 12u; });
	auto f2 = ext::async(ext::launch::deferred, [] { return 12u; });
	auto fall = ext::when_all(std::move(f1), std::move(f2));

	BOOST_CHECK(fall.is_ready());
	std::tie(f1, f2) = fall.get();
	BOOST_CHECK(f1.get() + f2.get() == 24);
}

BOOST_AUTO_TEST_SUITE_END()
