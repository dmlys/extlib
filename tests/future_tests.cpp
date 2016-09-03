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

	pi.set_value();

	BOOST_CHECK(count == 2);
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

BOOST_AUTO_TEST_SUITE_END()
