#include <iostream>
#include <future>
#include <thread>
#include <ctime>
#include <fstream>

#include "List.hpp"
#include "List_fine_graining.hpp"
#include "List_medium_graining.hpp"
#include "gnuplot.h"

using namespace ACIDList;
using namespace ACIDListMedium;
using namespace ACIDListFine;

int main() {
    Gnuplot plot;
	std::ofstream out_1, out_2, out_3;
	int n = 500000;
	out_1.open("spinlock.txt");
	out_2.open("medium_graining.txt");
	out_3.open("fine_graining.txt");
	
	std::cout << "SPINLOCK:" << std::endl << std::endl;
	
	for (int threadsnum = 1; threadsnum <= 8; threadsnum *= 2) {
		List<int> list;
		int iternum = (n / 2) / threadsnum;;
		std::vector<std::thread> threads;

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				for (int j = 0; j < n / threadsnum; ++j) list.push_back(j + th * n / threadsnum);
				}, i));
		}

		for (int k = 0; k < threadsnum; ++k) threads[k].join();
		std::condition_variable cv;

		threads.clear();
		auto startThread = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				auto it = list.begin();
				for (int j = 0; j < iternum; ++j) ++it;
				std::mutex mutex_;
				std::unique_lock<std::mutex> lck(mutex_);
				cv.wait(lck);
				for (int j = 0; j < iternum; ++j) {
					list.erase(it);
					++it;
				}
				}, i));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		cv.notify_all();

		for (int k = 0; k < threadsnum; ++k) threads[k].join();

		auto endThread = std::chrono::high_resolution_clock::now();
		auto timeThread = std::chrono::duration_cast<std::chrono::milliseconds>(endThread - startThread);

		if (out_1.is_open()) out_1 << threadsnum << " " << (double)timeThread.count() / 1000.0 - 1 << std::endl;

		std::cout << "NUMBER OF THREADS = " << threadsnum << std::endl;
		std::cout << "TIME = " << (double)timeThread.count() / 1000.0 - 1 << std::endl;
	}

	std::cout << std::endl << "MEDIUM GRAINING:" << std::endl << std::endl;

	for (int threadsnum = 1; threadsnum <= 8; threadsnum *= 2) {
		List_medium<int> list;
		int iternum = (n / 2) / threadsnum;;
		std::vector<std::thread> threads;

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				for (int j = 0; j < n / threadsnum; ++j) list.push_back(j + th * n / threadsnum);
				}, i));
		}

		for (int k = 0; k < threadsnum; ++k) threads[k].join();
		std::condition_variable cv;

		threads.clear();
		auto startThread = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				auto it = list.begin();
				for (int j = 0; j < iternum; ++j) ++it;
				std::mutex mutex_;
				std::unique_lock<std::mutex> lck(mutex_);
				cv.wait(lck);
				for (int j = 0; j < iternum; ++j) {
					list.erase(it);
					++it;
				}
				}, i));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		cv.notify_all();

		for (int k = 0; k < threadsnum; ++k) threads[k].join();

		auto endThread = std::chrono::high_resolution_clock::now();
		auto timeThread = std::chrono::duration_cast<std::chrono::milliseconds>(endThread - startThread);

		if (out_2.is_open()) out_2 << threadsnum << " " << (double)timeThread.count() / 1000.0 - 1 << std::endl;

		std::cout << "NUMBER OF THREADS = " << threadsnum << std::endl;
		std::cout << "TIME = " << (double)timeThread.count() / 1000.0 - 1 << std::endl;
	}

	std::cout << std::endl << "FINE GRAINING:" << std::endl << std::endl;

	for (int threadsnum = 1; threadsnum <= 8; threadsnum *= 2) {
		List_fine<int> list;
		int iternum = (n / 2) / threadsnum;;
		std::vector<std::thread> threads;

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				for (int j = 0; j < n / threadsnum; ++j) list.push_back(j + th * n / threadsnum);
				}, i));
		}

		for (int k = 0; k < threadsnum; ++k) threads[k].join();
		std::condition_variable cv;

		threads.clear();
		auto startThread = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < threadsnum; ++i) {
			threads.push_back(std::thread([&](int th) {
				auto it = list.begin();
				for (int j = 0; j < iternum; ++j) ++it;
				std::mutex mutex_;
				std::unique_lock<std::mutex> lck(mutex_);
				cv.wait(lck);
				for (int j = 0; j < iternum; ++j) {
					list.erase(it);
					++it;
				}
				}, i));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		cv.notify_all();

		for (int k = 0; k < threadsnum; ++k) threads[k].join();

		auto endThread = std::chrono::high_resolution_clock::now();
		auto timeThread = std::chrono::duration_cast<std::chrono::milliseconds>(endThread - startThread);

		if (out_3.is_open()) out_3 << threadsnum << " " << (double)timeThread.count() / 1000.0 - 1 << std::endl;

		std::cout << "NUMBER OF THREADS = " << threadsnum << std::endl;
		std::cout << "TIME = " << (double)timeThread.count() / 1000.0 - 1 << std::endl;
	}

	out_1.close();
	out_2.close();
	out_3.close();
    
	plot("plot 'spinlock.txt' with lines, 'medium_graining.txt' with lines, 'fine_graining.txt' with lines");

	return 0;
}
