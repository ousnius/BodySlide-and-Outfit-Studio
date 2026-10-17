/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

// Runs functions on the main thread for callers on other threads, and waits for them to finish.
// The owner calls ProcessPending on the main thread whenever the wake function is called.
class MainThreadDispatcher {
public:
	enum class Result { Done, TimedOut, ShutDown };

	// Has to be created on the main thread. wake is called from other threads and has to make
	// the main thread call ProcessPending soon, e.g. through wxEvtHandler::CallAfter.
	explicit MainThreadDispatcher(std::function<void()> wake);
	~MainThreadDispatcher();

	MainThreadDispatcher(const MainThreadDispatcher&) = delete;
	MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;

	// Runs fn on the main thread and waits until it has run. On the main thread itself fn runs
	// right away. An exception thrown by fn is thrown again here.
	// After a timeout fn either doesn't run at all or is already running and finishes later, so it
	// has to own what it uses instead of referring to the caller's local variables.
	Result Run(std::function<void()> fn, std::chrono::milliseconds timeout);

	// Runs the queued functions. Main thread only.
	void ProcessPending();

	// Waiting and later calls return ShutDown without running.
	void Shutdown();

	bool IsMainThread() const { return std::this_thread::get_id() == mainThread; }

private:
	// Fails the calls that were still waiting when the dispatcher shut down
	struct ShutDownError {};

	struct Call {
		std::function<void()> fn;
		std::promise<void> done;
		bool started = false;
		bool abandoned = false;
	};

	std::thread::id mainThread;
	std::function<void()> wake;

	std::mutex mutex;
	std::deque<std::shared_ptr<Call>> queue;
	bool shutDown = false;
};
