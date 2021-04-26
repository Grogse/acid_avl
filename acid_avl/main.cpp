#include <iostream>
#include <map>
#include <ctime>
#include <vector>
#include "AVLtree.hpp"

using namespace std;
using namespace AVLtree;

int main() {

	int n = 1000;
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

	//EXPECT_TRUE(tree.height() <= 1.44 * log2(n));

	for (int i = 0; i < n; ++i) {
		int key = rand() % n;
		int val;

		try {
			val = truetree.at(key);
		}
		catch (const std::out_of_range& e) {
			continue;
		}

		//EXPECT_TRUE(tree.at(key) == val);
	}

	iter = tree.begin();
	int key_ = iter.get_key();
	while (++iter != tree.end()) {
		if (key_ <= iter.get_key()) cout << "SUCK COCK ";
	}

	return 0;
}
