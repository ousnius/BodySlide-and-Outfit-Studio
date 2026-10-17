#include "../src/utils/MainThreadDispatcher.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

namespace {
// Plays the main thread's event loop: processes queued calls until told to stop
void PumpUntil(MainThreadDispatcher& dispatcher, const std::atomic<bool>& stop) {
	while (!stop) {
		dispatcher.ProcessPending();
		std::this_thread::sleep_for(1ms);
	}
}
} // namespace

TEST_CASE("Calls from other threads run on the main thread", "[MainThreadDispatcher]") {
	std::atomic<int> wakes{0};
	MainThreadDispatcher dispatcher([&]() { wakes++; });
	const auto mainId = std::this_thread::get_id();

	std::atomic<bool> stop{false};
	std::thread::id ranOn;
	MainThreadDispatcher::Result result = MainThreadDispatcher::Result::TimedOut;

	std::thread worker([&]() {
		result = dispatcher.Run([&]() { ranOn = std::this_thread::get_id(); }, 5s);
		stop = true;
	});

	PumpUntil(dispatcher, stop);
	worker.join();

	CHECK(result == MainThreadDispatcher::Result::Done);
	CHECK(ranOn == mainId);
	CHECK(wakes > 0);
}

TEST_CASE("Calls on the main thread run right away", "[MainThreadDispatcher]") {
	MainThreadDispatcher dispatcher([]() {});
	bool ran = false;
	CHECK(dispatcher.Run([&]() { ran = true; }, 0ms) == MainThreadDispatcher::Result::Done);
	CHECK(ran);
}

TEST_CASE("Exceptions reach the caller", "[MainThreadDispatcher]") {
	MainThreadDispatcher dispatcher([]() {});
	std::atomic<bool> stop{false};
	bool caught = false;

	std::thread worker([&]() {
		try {
			dispatcher.Run([]() { throw std::runtime_error("failed"); }, 5s);
		}
		catch (const std::runtime_error&) {
			caught = true;
		}
		stop = true;
	});

	PumpUntil(dispatcher, stop);
	worker.join();
	CHECK(caught);
}

TEST_CASE("A call that times out before it starts never runs", "[MainThreadDispatcher]") {
	MainThreadDispatcher dispatcher([]() {});
	auto ran = std::make_shared<std::atomic<bool>>(false);

	MainThreadDispatcher::Result result = MainThreadDispatcher::Result::Done;
	std::thread worker([&]() { result = dispatcher.Run([ran]() { *ran = true; }, 20ms); });
	worker.join();

	// The main thread only gets to the queue after the caller gave up
	dispatcher.ProcessPending();
	CHECK(result == MainThreadDispatcher::Result::TimedOut);
	CHECK_FALSE(*ran);
}

TEST_CASE("Shutting down releases waiting callers", "[MainThreadDispatcher]") {
	std::atomic<bool> queued{false};
	MainThreadDispatcher dispatcher([&]() { queued = true; });

	MainThreadDispatcher::Result result = MainThreadDispatcher::Result::Done;
	std::thread worker([&]() { result = dispatcher.Run([]() {}, 10s); });

	while (!queued)
		std::this_thread::sleep_for(1ms);

	dispatcher.Shutdown();
	worker.join();
	CHECK(result == MainThreadDispatcher::Result::ShutDown);
	CHECK(dispatcher.Run([]() {}, 0ms) == MainThreadDispatcher::Result::ShutDown);
}
