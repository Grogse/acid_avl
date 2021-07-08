#include "pch.h"
#include <ctime>
#include "../acid_avl/AVLtree.hpp"

using namespace std;
using namespace AVLtree;

TEST(Modifiers, RandomInsert) {
	int n = 10000, threads_count = 8;
	AVL<int, int> tree;
	AVLiterator<int, int> iter_1, iter_2;
	std::vector<std::thread> threads;

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](int th) {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n;
				int value = rand();
				tree.insert(pair<int, int>(key, value));
			}
			}, i));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	iter_1 = tree.begin();
	int key_1 = iter_1.get_key();

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](auto it) {
			while (++it != tree.end()) EXPECT_TRUE(key_1 <= it.get_key());
			}, iter_1));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	iter_2 = tree.end();
	iter_2--;
	int key_2 = iter_2.get_key();

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](auto it) {
			while (--it != tree.begin()) EXPECT_TRUE(key_2 >= it.get_key());
			}, iter_2));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
}

/*TEST(Modifiers, RandomErase) {
	int n = 10000, threads_count = 8;
	AVL<int, int> tree;
	AVLiterator<int, int> iter_1, iter_2;
	std::vector<std::thread> threads;

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n;
				int value = rand();
				tree.insert(pair<int, int>(key, value));
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n / 2;
				tree.erase(key);
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	iter_1 = tree.begin();
	int key_ = iter_1.get_key();

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](auto it) {
			while (++it != tree.end()) EXPECT_TRUE(key_ <= it.get_key());
			}, iter_1));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	iter_2 = tree.end();
	iter_2--;
	int key_2 = iter_2.get_key();

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](auto it) {
			while (--it != tree.begin()) EXPECT_TRUE(key_2 >= it.get_key());
			}, iter_2));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
}*/

TEST(Modifiers, ConditionVariable) {
	int n = 10000, threads_count = 8;
	AVL<int, int> tree;
	std::vector<std::thread> checkers;

	std::shared_mutex mut;
	std::condition_variable_any convar;
	std::vector<int> savedValues;
	std::atomic<bool> dataReady{false};
	std::atomic<int> consumed{0};
	std::vector<int> wrongValues;
	
	for (int i = 0; i < threads_count; ++i) {
		checkers.push_back(std::thread([&]() {
			std::shared_lock<std::shared_mutex> guard(mut);
			convar.wait(guard, [&] { return dataReady.load(); });

			auto it = tree.begin();
			for (int i = 0; i < n / (threads_count * 3); ++i) {
				it++;
				tree.erase(it.get_key());
			}

			dataReady = false;
			consumed++;
			convar.notify_all();
			convar.wait(guard, [&]() {return dataReady == true; });

			it = tree.begin();
			while (it != tree.end()) it++;
			auto val = it.get_value() + it.get_value() % 10;
			savedValues.push_back(val);

			dataReady = false;
			consumed++;
			convar.notify_all();
			}));
	}

	std::thread producer([&]() {
		srand(time(0));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::unique_lock<std::shared_mutex> guard(mut);

		for (int i = 0; i < n; ++i) {
			tree.insert(std::pair<int, int>(rand(), rand()));
		}
		
		dataReady = true;
		convar.notify_all();
		convar.wait(guard, [&]() { return consumed.load() == threads_count; });

		tree.insert(std::pair<int, int>(rand(), rand()));
		dataReady = true;
		consumed = 0;

		convar.notify_all();
		convar.wait(guard, [&]() { return consumed.load() == threads_count; });


		auto correctValue = savedValues[0];
		std::copy_if(savedValues.begin(), savedValues.end(), std::back_inserter(wrongValues), 
			[&](int i) {return i != correctValue; });
		});

	for (auto& th : checkers) th.join();
	producer.join();

	EXPECT_TRUE(wrongValues.size() == 0);
}

TEST(Iterator, Invalidation) {
	AVL<int, int> tree;
	AVLiterator<int, int> iter;
	tree.insert(std::pair<int, int>(1, 2));
	tree.insert(std::pair<int, int>(3, 4));
	tree.insert(std::pair<int, int>(5, 6));

	iter = tree.begin();
	iter++;

	EXPECT_TRUE(iter.get_key() == 3);
	EXPECT_TRUE(iter.get_value() == 4);

	tree.erase(3);

	iter++;

	EXPECT_TRUE(iter.get_key() == 5);
	EXPECT_TRUE(iter.get_value() == 6);
}

/*TEST(Iterator, RandomInvalidation) {
	int n = 10000, threads_count = 8;
	AVL<int, int> tree;
	std::vector<std::thread> threads;

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n;
				int value = rand();
				tree.insert(pair<int, int>(key, value));
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	std::vector<AVLiterator<int, int>> its;

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n;
				int value = rand();
				tree.insert(pair<int, int>(key, value));
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();
	
	for (int i = 0; i < n; ++i) {
		AVLiterator<int, int> iter = tree.begin();
		int m = std::rand() % (n / 2);

		for (int j = 1; j < m; ++j) {
			iter++;
			if (iter == tree.end()) break;
		}

		its.push_back(iter);
	}

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				int key = rand() % n / 2;
				tree.erase(key);
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();

	for (auto it : its) {
		while (it != tree.end()) {
			it++;
		}
	}
}*/

/*TEST(Iterator, InsertEraseInvalidation) {
	int n = 10000, threads_count = 8;
	AVL<int, int> tree;
	AVLiterator<int, int> iter;
	std::vector<std::thread> threads;

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&] {
			srand(time(0));
			for (int j = 0; j < n; ++j) {
				if (j % 3 == 0) {
					int key = rand() % n;
					tree.erase(key);
				}
				else {
					int key = rand() % n;
					int value = rand();
					tree.insert(pair<int, int>(key, value));
				}
			}}));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
	threads.clear();

	iter = tree.begin();
	int key_ = iter.get_key();

	for (int i = 0; i < threads_count; ++i) {
		threads.push_back(std::thread([&](auto it) {
			srand(time(0));
			while (it != tree.end()) {
				if (rand() % 3 == 0) it--;
				else it++;
			}}, iter));
	}

	for (int i = 0; i < threads_count; ++i) threads[i].join();
}*/
