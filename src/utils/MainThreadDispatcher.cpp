/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "MainThreadDispatcher.h"

MainThreadDispatcher::MainThreadDispatcher(std::function<void()> inWake)
	: mainThread(std::this_thread::get_id())
	, wake(std::move(inWake)) {}

MainThreadDispatcher::~MainThreadDispatcher() {
	Shutdown();
}

MainThreadDispatcher::Result MainThreadDispatcher::Run(std::function<void()> fn, std::chrono::milliseconds timeout) {
	if (IsMainThread()) {
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (shutDown)
				return Result::ShutDown;
		}

		fn();
		return Result::Done;
	}

	auto call = std::make_shared<Call>();
	call->fn = std::move(fn);
	std::future<void> done = call->done.get_future();

	{
		std::lock_guard<std::mutex> lock(mutex);
		if (shutDown)
			return Result::ShutDown;
		queue.push_back(call);
	}

	if (wake)
		wake();

	if (done.wait_for(timeout) == std::future_status::timeout) {
		std::lock_guard<std::mutex> lock(mutex);
		if (!call->started) {
			call->abandoned = true;
			return Result::TimedOut;
		}
	}

	// It started before the timeout ran out or finished in time, either way wait for the result
	try {
		done.get();
	}
	catch (const ShutDownError&) {
		return Result::ShutDown;
	}

	return Result::Done;
}

void MainThreadDispatcher::ProcessPending() {
	for (;;) {
		std::shared_ptr<Call> call;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (queue.empty())
				return;

			call = queue.front();
			queue.pop_front();
			if (call->abandoned)
				continue;

			call->started = true;
		}

		try {
			call->fn();
			call->done.set_value();
		}
		catch (...) {
			call->done.set_exception(std::current_exception());
		}
	}
}

void MainThreadDispatcher::Shutdown() {
	std::deque<std::shared_ptr<Call>> pending;
	{
		std::lock_guard<std::mutex> lock(mutex);
		shutDown = true;
		pending.swap(queue);
	}

	// Wakes the callers that are still waiting for these
	for (auto& call : pending)
		call->done.set_exception(std::make_exception_ptr(ShutDownError()));
}
