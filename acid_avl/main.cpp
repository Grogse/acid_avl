#include <iostream>
#include <vector>
#include "AVLtree.hpp"

using namespace std;
using namespace AVLtree;

int main() {

	int n = 40;
	srand(2);
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

		//cout << iter.get_key() << endl;
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

	cout << &its;

	/*for (int i = 0; i < its.size(); ++i) {
		cout << its[i].get_count() << endl;
	}*/

	return 0;
}
