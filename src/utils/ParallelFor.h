/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <utility>
#include <vector>

constexpr size_t parallelForDefaultMinItemsPerWorker = 4096;
constexpr unsigned int parallelForMaxWorkerCount = 16;

inline unsigned int GetParallelWorkerCount(size_t itemCount, size_t minItemsPerWorker) {
	if (minItemsPerWorker == 0)
		minItemsPerWorker = 1;

	if (itemCount / minItemsPerWorker < 2)
		return 1;

	unsigned int hardwareWorkers = std::thread::hardware_concurrency();
	if (hardwareWorkers <= 1)
		return 1;

	size_t itemLimitedWorkers = itemCount / minItemsPerWorker;
	if (itemLimitedWorkers < 2)
		return 1;

	return std::min({hardwareWorkers, static_cast<unsigned int>(itemLimitedWorkers), parallelForMaxWorkerCount});
}

inline unsigned int GetParallelWorkerCount(size_t itemCount) {
	return GetParallelWorkerCount(itemCount, parallelForDefaultMinItemsPerWorker);
}

template <typename Func>
void ParallelFor(size_t itemCount, size_t minItemsPerWorker, Func&& func) {
	unsigned int workerCount = GetParallelWorkerCount(itemCount, minItemsPerWorker);
	if (workerCount <= 1) {
		if (itemCount > 0)
			func(0, itemCount);
		return;
	}

	std::vector<std::thread> workers;
	workers.reserve(workerCount - 1);
	size_t chunkSize = (itemCount + workerCount - 1) / workerCount;
	size_t startIndex = 0;

	for (unsigned int workerIndex = 1; workerIndex < workerCount; workerIndex++) {
		size_t endIndex = std::min(itemCount, startIndex + chunkSize);
		workers.emplace_back([startIndex, endIndex, &func]() {
			if (startIndex < endIndex)
				func(startIndex, endIndex);
		});
		startIndex = endIndex;
	}

	if (startIndex < itemCount)
		func(startIndex, itemCount);

	for (auto& worker : workers)
		worker.join();
}

template <typename Func>
void ParallelFor(size_t itemCount, Func&& func) {
	ParallelFor(itemCount, parallelForDefaultMinItemsPerWorker, std::forward<Func>(func));
}

template <typename Func>
void ParallelForDynamic(size_t itemCount, size_t minItemsPerWorker, size_t chunkSize, Func&& func) {
	unsigned int workerCount = GetParallelWorkerCount(itemCount, minItemsPerWorker);
	if (workerCount <= 1) {
		if (itemCount > 0)
			func(0, itemCount);
		return;
	}

	if (chunkSize == 0)
		chunkSize = 1;

	std::atomic<size_t> nextIndex = 0;
	auto runWorker = [&]() {
		for (;;) {
			size_t startIndex = nextIndex.fetch_add(chunkSize, std::memory_order_relaxed);
			if (startIndex >= itemCount)
				break;

			size_t endIndex = std::min(itemCount, startIndex + chunkSize);
			func(startIndex, endIndex);
		}
	};

	std::vector<std::thread> workers;
	workers.reserve(workerCount - 1);
	for (unsigned int workerIndex = 1; workerIndex < workerCount; workerIndex++)
		workers.emplace_back(runWorker);

	runWorker();

	for (auto& worker : workers)
		worker.join();
}

template <typename Func>
void ParallelForDynamic(size_t itemCount, Func&& func) {
	ParallelForDynamic(itemCount, parallelForDefaultMinItemsPerWorker, parallelForDefaultMinItemsPerWorker, std::forward<Func>(func));
}
