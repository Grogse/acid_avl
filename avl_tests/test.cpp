#include "pch.h"
#include <ctime>
#include "../acid_avl/AVLtree.hpp"

using namespace std;
using namespace AVLtree;

TEST(Modifiers, Insert) {
	int n = 7000;
	srand(time(0));
	AVL<int, int> tree;
	AVLiterator<int, int> iter;

	for (int i = 0; i < n; ++i) tree.insert(pair<int, int>(i, rand()));

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	iter = tree.begin();
	int key = iter.get_key();

	while (++iter != tree.end()) EXPECT_TRUE(key <= iter.get_key());
}

TEST(Modifiers, RandomInsert) {
	int n = 7000;
	srand(time(0));
	AVL<int, int> tree;
	map<int, int> truetree;
	AVLiterator<int, int> iter;

	for (int i = 0; i < n; ++i) {
		int key = rand() % n;
		int value = rand();
		tree.insert(pair<int, int>(key, value));
		truetree.insert(pair<int, int>(key, value));
	}

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	for (int i = 0; i < n; ++i) {
		int key = rand() % n;
		int val;

		try {
			val = truetree.at(key);
		}
		catch (const std::out_of_range& e) {
			continue;
		}

		EXPECT_TRUE(tree.at(key) == val);
	}

	iter = tree.begin();
	int key_ = iter.get_key();
	while (++iter != tree.end()) EXPECT_TRUE(key_ <= iter.get_key());
}

TEST(Modifiers, RandomErase) {
	int n = 7000;
	srand(time(0));
	AVL<int, int> tree;
	map<int, int> truetree;
	AVLiterator<int, int> iter;

	for (int i = 0; i < n; ++i) {
		int value = rand();
		tree.insert(pair<int, int>(i, value));
		truetree.insert(pair<int, int>(i, value));
	}

	EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	for (int i = 0; i < n; ++i) {
		auto key = std::rand() % 5000;
		tree.erase(key);
		truetree.erase(key);
	}

	for (int i = 0; i < n; ++i) {
		int val;

		try {
			val = truetree.at(i);
		}
		catch (const std::out_of_range& e) {
			continue;
		}

		EXPECT_TRUE(tree.at(i) == val);
	}

	iter = tree.begin();
	int key_ = iter.get_key();
	while (++iter != tree.end()) EXPECT_TRUE(key_ <= iter.get_key());
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

TEST(Iterator, RandomInvalidation) {
	int n = 7000;
	srand(time(0));
	AVL<int, int> tree;

	for (int i = 0; i < n; ++i) {
		int value = rand();
		tree.insert(pair<int, int>(i, value));
	}

	std::vector<AVLiterator<int, int>> its;

	for (int i = 0; i < n; ++i) {
		AVLiterator<int, int> iter = tree.begin();
		int m = std::rand() % (n / 2);

		for (int j = 1; j < m; ++j) {
			iter++;
			if (iter == tree.end()) break;
		}

		its.push_back(iter);
	}

	for (int i = 0; i < n; ++i) {
		int key = std::rand() % n;
		tree.erase(key);
	}

	for (auto it : its) {
		while (it != tree.end()) {
			it++;
		}
	}
}
