#define _CRTDBG_MAP_ALLOC  
#include <iostream>
#include <map>
#include <ctime>
#include <vector>
#include "AVLtree.hpp"

using namespace std;
using namespace AVLtree;

int main() {
	int n = 10000, threads_count = 8;
	srand(time(0));
	AVL<int, int> tree;
	AVLiterator<int, int> iter;

	for (int j = 0; j < n; ++j) {
		int key = rand() % n;
		int value = rand();
		tree.insert(pair<int, int>(key, value));
	}

	iter = tree.end();
	iter--;
	int key_ = iter.get_key();
	
	while (--iter != tree.begin()) {
		if (!(key_ >= iter.get_key())) cout << "SUCK COCK\n";
	}
	
	return 0;
}
