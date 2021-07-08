#include "pch.h"
#include "../acid_list/List.hpp"

#include <iostream>
#include <future>
#include <thread>
#include <ctime>
#include <condition_variable>

using namespace ACIDList;

TEST(Multithreading, PushErase) {
	List<int> list;
	std::vector<std::thread> threads;
	int threadsnum = 8, n = 1000, deln = 100;

	for (int i = 0; i < n * threadsnum; ++i) {
		threads.push_back(std::thread(&List<int>::push_back, &list, i));
	}

	for (int i = 0; i < n * threadsnum; ++i) threads[i].join();

	EXPECT_TRUE(list.size() == static_cast<size_t>(n * threadsnum));

	for (int i = 0; i < n * threadsnum; ++i) EXPECT_TRUE(list.find(i).get() == i);
	threads.clear();

	for (int i = 0; i < threadsnum; ++i) {
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < deln; ++j) {
				auto it = list.find(j + th * deln);
				list.erase(it);
			}
			}, i));
	}

	for (int i = 0; i < threadsnum; ++i) threads[i].join();

	for (size_t i = deln * threadsnum; i < list.size(); ++i) {
		EXPECT_TRUE(list.find(i).get() == i);
	}
}

TEST(Multithreading, MediumGrained) {
	List<int> list;
	std::vector<std::thread> threads;
	int count = 0, threadsnum = 8, n = 1000;
	auto startThread = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < threadsnum; ++i) {
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < n * 2; ++j) list.push_back(j + th * n);
			}, i));
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < n; ++j) list.push_front(j + (th + threadsnum) * n);
			}, i));
	}

	for (int k = 0; k < threadsnum * 2; ++k) threads[k].join();
	threads.clear();

	threads.push_back(std::thread([&]() {
		for (int j = 0; j < n; ++j) {
			auto it = list.begin();
			list.erase(it);
		}
		}));

	threads[0].join();

	auto endThreaded = std::chrono::high_resolution_clock::now();
	auto timeThreaded = std::chrono::duration_cast<std::chrono::milliseconds>(endThreaded - startThread);
	std::cout << (double)timeThreaded.count() / 1000.0 << std::endl;

	EXPECT_TRUE(list.size() >= static_cast<size_t>(n * threadsnum));
}
TEST(Multithreading, Insert) {
	List<int> list;
	std::vector<std::thread> threads;
	int count = 0, threadsnum = 8, n = 1000;
	auto startThread = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < threadsnum; ++i) {
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < n; ++j) {
				auto it = list.begin();
				list.insert(it, j + th * n);
			}
			}, i));
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < n; ++j) list.push_front(j + (th + threadsnum) * n);
			}, i));
	}

	auto endThread = std::chrono::high_resolution_clock::now();
	auto timeThreaded = std::chrono::duration_cast<std::chrono::milliseconds>(endThread - startThread);
	std::cout << (double)timeThreaded.count() / 1000.0 << std::endl;

	for (int k = 0; k < threadsnum * 2; ++k) threads[k].join();
	threads.clear();

	for (int i = 0; i < threadsnum; ++i) {
		threads.push_back(std::thread([&](int th) {
			auto it = list.begin();
			auto last = list.end();
			while (it != last) {
				++it;
			}
			}, i));
	}

	for (int k = 0; k < threadsnum; ++k) threads[k].join();
}

TEST(Multithreading, TimeTest) {
	for (int threadsnum = 1; threadsnum <= 4; threadsnum *= 2) {
		std::vector<std::thread> threads;
		List<int> list;
		int count = 0, n = 5000000, deln = n / 2;
		auto startThread = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				for (int j = 0; j < n / threadsnum; ++j) list.push_back(j + th * n / threadsnum);
				}, i));
		}

		for (int k = 0; k < threadsnum; ++k) threads[k].join();
		EXPECT_TRUE(list.size() == n);
		threads.clear();

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				for (int j = 0; j < deln / threadsnum; ++j) {
					auto it = list.begin();
					list.erase(it);
				}
				}, i));
		}

		for (int k = 0; k < threadsnum; ++k) threads[k].join();

		EXPECT_TRUE(list.size() >= static_cast<size_t>(n - deln));

		auto endThread = std::chrono::high_resolution_clock::now();
		auto timeThreaded = std::chrono::duration_cast<std::chrono::milliseconds>(endThread - startThread);

		std::cout << "NUMBER OF THREADS = " << threadsnum << std::endl;
		std::cout << "TIME = " << (double)timeThreaded.count() / 1000.0 << std::endl;
	}
}

TEST(Multithreading, TimeIteration) {
	srand(time(nullptr));
	List<int> list;
	std::condition_variable cv;
	std::vector<std::thread> threads;
	int count = 0, threadsnum = 8, n = 500000, deln = n / 2;
	auto startThreaded = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < threadsnum; ++i) {
		threads.push_back(std::thread([&](int th) {
			for (int j = 0; j < n / threadsnum; ++j) list.push_back(j + th * n / threadsnum);
			}, i));
	}

	for (int k = 0; k < threadsnum; ++k) threads[k].join();

	EXPECT_TRUE(list.size() == n);
	threads.clear();

	for (int i = 0; i < threadsnum / 2; ++i) {
		threads.push_back(std::thread([&](int th) {
			std::mutex mutex_;
			std::unique_lock<std::mutex> lck(mutex_);
			cv.wait(lck);
			for (int j = 0; j < deln / threadsnum / 2; ++j) {
				auto it = list.begin();
				list.erase(it);
			}
			}, i));
		
		threads.push_back(std::thread([&](int th) {
			std::mutex mutex_;
			std::unique_lock<std::mutex> lck(mutex_);
			cv.wait(lck);
			auto it = list.begin();
			int numberOfIterations = rand() % n;
			for (int j = 0; j < numberOfIterations; ++j) ++it;
			}, i));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	cv.notify_all();

	for (int k = 0; k < threadsnum; ++k) threads[k].join();

	EXPECT_TRUE(list.size() >= static_cast<size_t>(n - deln));
}