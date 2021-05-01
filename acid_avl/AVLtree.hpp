#pragma once

#include <utility>
#include <stdexcept>

namespace AVLtree {
	std::size_t compare(std::size_t a, std::size_t b) {
		return (a > b) ? a : b;
	}

	enum state_for_iterator {
		OCCUPIED,
		REMOVED,
		LAST,
		FREE,
		ROOT,
		END,
	};

	template<typename KEY, typename DATA>
	class Node {
	protected:
		using key_type = KEY;
		using data_type = DATA;
		using state_for_node = state_for_iterator;
		using value_type = std::pair<const key_type, data_type>;
		using size_type = std::size_t;

		template<typename KEY, typename DATA>
		friend class AVL;

		template<typename KEY, typename DATA>
		friend class AVLiterator;

		Node() : parent(nullptr), left(nullptr), right(nullptr), height(0), ref_count(0),
			state(FREE) {}

		Node(const value_type &val) : Node() {
			new (&this->data) value_type(std::move(val)); //placement new
			this->state = OCCUPIED;
			this->height = 1;
		}

		~Node() {
			if (this != nullptr) {
				this->parent = nullptr;
				if (this->left) delete this->left;
				if (this->right) delete this->right;
			}
		}

		value_type data;

		Node *parent;
		Node *left;
		Node *right;

		size_type height;
		size_type ref_count;
		state_for_node state;
	};

	template<typename KEY, typename DATA>
	class AVLiterator {
	public:
		using key_type = KEY;
		using data_type = DATA;
		using node_type = Node<key_type, data_type>;
		using pointer = node_type*;
		using reference = node_type&;
		using value_type = std::pair<const key_type, data_type>;

		template<typename KEY, typename DATA>
		friend class AVL;

		AVLiterator() noexcept : ptr(nullptr), state(FREE) {}

		AVLiterator(const AVLiterator& other) noexcept {
			this->ptr = other.ptr;
			this->ptr->ref_count += 1;
			this->state = other.state;
		}

		key_type get_key() {
			return this->ptr->data.first;
		}

		data_type get_value() {
			return this->ptr->data.second;
		}

		std::size_t get_count() {
			return this->ptr->ref_count;
		}

		AVLiterator operator++(int) {
			AVLiterator *tmp = new AVLiterator();
			*tmp = *this;
			tmp->ptr->ref_count -= 1;
			++*this;

			return *tmp;
		}

		AVLiterator& operator++() {
			if (this->ptr != nullptr) {
				if (this->ptr->state == REMOVED) {
					if (this->ptr->right != nullptr) {
						this->ptr->ref_count -= 1;
						node_type *tmp = this->ptr;
						node_type *t = this->ptr->right;
						this->ptr = t;
						this->ptr->ref_count += 1;
						destroy_node(tmp);

						return *this;
					}
					else {
						if ((this->ptr->parent != nullptr) && (this->ptr->data.first < this->ptr->parent->data.first)) {
							this->ptr->ref_count -= 1;
							node_type *tmp = this->ptr;
							this->ptr = this->ptr->parent;
							this->ptr->ref_count += 1;
							destroy_node(tmp);

							return *this;
						}
						else {
							node_type *temp = this->ptr->parent;
							while (temp->state == REMOVED) {
								if ((temp->right != nullptr) && (temp->right->data.first > temp->data.first)) temp = temp->right;
								else temp = temp->parent;
							}

							this->ptr->ref_count -= 1;
							node_type *tmp = this->ptr;
							this->ptr = temp;
							this->ptr->ref_count += 1;
							destroy_node(tmp);
						}
					}

					return *this;
				}

				if (this->ptr->state == LAST) {
					this->ptr->ref_count -= 1;
					this->state = END;

					return *this;
				}

				if (this->state == END) {
					return *this;
				}
				else {
					if (this->ptr->right == nullptr) {
						if (this->ptr->data.first < this->ptr->parent->data.first) {
							this->ptr->ref_count -= 1;
							this->ptr = this->ptr->parent;
							this->ptr->ref_count += 1;
						}
						else {
							node_type *temp = this->ptr->parent;
							while (temp->data.first < this->ptr->data.first) temp = temp->parent;

							this->ptr->ref_count -= 1;
							this->ptr = temp;
							this->ptr->ref_count += 1;
						}
					}
					else {
						this->ptr->ref_count -= 1;
						this->ptr = find_min(this->ptr->right);
						this->ptr->ref_count += 1;
					}
				}
			}

			return *this;
		}

		AVLiterator operator--(int) {
			AVLiterator *tmp = new AVLiterator();
			*tmp = *this;
			--*this;

			return *tmp;
		}

		AVLiterator& operator--() {
			if (this->ptr != nullptr) {
				if (this->ptr->state == REMOVED) {
					if ((this->ptr->parent != nullptr) && (this->ptr->data.first < this->ptr->parent->data.first)) {
						if (this->ptr->left != nullptr) {
							this->ptr->ref_count -= 1;
							node_type *tmp = this->ptr;
							this->ptr = this->ptr->left;
							this->ptr->ref_count += 1;
							destroy_node(tmp);
						}

						return *this;
					}
					else {
						if (this->ptr->parent != nullptr) {
							this->ptr->ref_count -= 1;
							node_type *tmp = this->ptr;
							this->ptr = this->ptr->parent;
							this->ptr->ref_count += 1;
							destroy_node(tmp);
						}

						return *this;
					}
				}

				if (this->ptr == find_begin(this->ptr)) return *this;

				if (this->ptr->state == END) {
					this->ptr->ref_count += 1;
					this->ptr->state = OCCUPIED;

					return *this;
				}

				if ((this->ptr->left == nullptr) && (this->ptr->right == nullptr)) {
					node_type *tmp = this->ptr->parent;
					while (tmp->data.first > this->ptr->data.first) tmp = tmp->parent;

					this->ptr->ref_count -= 1;
					this->ptr = tmp;
					this->ptr->ref_count += 1;
				}
				else {
					if (this->ptr->left != nullptr) {
						this->ptr->ref_count -= 1;
						this->ptr = find_max(this->ptr->left);
						this->ptr->ref_count += 1;
					}
					else {
						node_type *tmp = this->ptr->parent;
						while (tmp->data.first > this->ptr->data.first) tmp = tmp->parent;

						this->ptr->ref_count -= 1;
						this->ptr = tmp;
						this->ptr->ref_count += 1;
					}
				}
			}

			AVLiterator *tmp = new AVLiterator();
			*tmp = *this;

			return *tmp;
		}

		void operator=(const AVLiterator &iter) {
			if (this->ptr != nullptr) {
				this->ptr->ref_count -= 1;
				destroy_node(this->ptr);
			}

			this->ptr = iter.ptr;
			this->ptr->ref_count += 1;
			this->state = iter.state;
		}

		reference operator*() const {
			return *this->ptr;
		}

		pointer operator->() const {
			return this->ptr;
		}

		friend bool operator==(const AVLiterator &left, const AVLiterator &right) {
			return left.ptr == right.ptr;
		}

		friend bool operator!=(const AVLiterator &left, const AVLiterator &right) {
			return left.ptr != right.ptr;
		}

		~AVLiterator() {
			if (this->ptr != nullptr) {
				this->ptr->ref_count -= 1;
				this->ptr = nullptr;
			}
		}

	protected:
		void destroy_node(node_type *node) {
			if (node->ref_count == 0) {
				if (node->parent != nullptr) {
					node->parent->ref_count -= 1;
					destroy_node(node->parent);
					node->parent = nullptr;
				}

				if (node->left != nullptr) {
					node->left->ref_count -= 1;
					destroy_node(node->left);
					node->left = nullptr;
				}

				if (node->right != nullptr) {
					node->right->ref_count -= 1;
					destroy_node(node->right);
					node->right = nullptr;
				}

				delete node;
			}
		}

		node_type* find_min(node_type *node) {
			node_type *tmp = node;
			while (tmp->left != nullptr) tmp = tmp->left;

			return tmp;
		}

		node_type* find_max(node_type *node) {
			node_type *tmp = node;
			while (tmp->right != nullptr) tmp = tmp->right;

			return tmp;
		}

		node_type* find_begin(node_type *node) {
			if (node->state == ROOT) return find_min(node);
			else {
				node_type *tmp = node;
				while (tmp->parent != nullptr) tmp = tmp->parent;
				return find_min(node);
			}
		}

		pointer ptr;
		state_for_iterator state;
	};

	template<typename KEY, typename DATA>
	class AVL {
	public:
		using key_type = KEY;
		using data_type = DATA;
		using node_type = Node<key_type, data_type>;
		using iterator = AVLiterator<key_type, data_type>;
		using value_type = std::pair<const key_type, data_type>;
		using size_type = std::size_t;

		AVL() : root(new node_type()), sentinel(), size_(0) {}

		AVL(const value_type &value) : root(new node_type(value)), sentinel(), size_(1) {
			this->state = ROOT;
			this->ref_count += 1;
			this->sentinel.ptr = root;
			this->sentinel.state = END;
		}

		value_type min() {
			return find_min(this->root)->data;
		}

		value_type	max() {
			return find_max(this->root)->data;
		}

		void insert(const value_type &value) {
			set_root(push(this->root, this->root, value));
		}

		void erase(const key_type &key) {
			set_root(remove(this->root, key));
		}

		iterator begin() {
			iterator iter;
			iter.ptr = find_min(this->root);
			iter.state = OCCUPIED;

			return iter;
		}

		iterator end() {
			return this->sentinel;
		}

		data_type at(const key_type &key) {
			node_type *tmp = find(key);

			if (tmp != nullptr) return tmp->data.second;
			throw std::out_of_range("key out of range");
		}

		void print() {
			print_avl(this->root);
		}

		size_type size() {
			return this->size_;
		}

		size_type height() {
			return this->root->height;
		}

		~AVL() {
			if (this->root != nullptr) {
				delete this->root;
				this->sentinel.ptr = nullptr;
			}
		}

	private:
		void print_avl(node_type *node) {
			if (node != nullptr) {
				print_avl(node->left);
				std::cout << node->data.first << " ";
				print_avl(node->right);
			}
		}

		size_type node_height(node_type *node) {
			if (node == nullptr) return 0;
			return node->height;
		}

		int get_balance(node_type *node) {
			if (node == nullptr) return 0;
			return static_cast<int>(node_height(node->left)) - static_cast<int>(node_height(node->right));
		}

		void set_root(node_type *node) {
			this->root = node;
		}

		node_type* find_min(node_type *node) {
			node_type *tmp = node;
			while (tmp->left != nullptr) tmp = tmp->left;
			return tmp;
		}

		node_type* find_max(node_type *node) {
			node_type *tmp = node;
			while (tmp->right != nullptr) tmp = tmp->right;
			return tmp;
		}

		node_type* find(const key_type &key) {
			node_type *tmp = this->root;

			while (tmp != nullptr) {
				if (key == tmp->data.first) return tmp;
				if (key < tmp->data.first) tmp = tmp->left;
				if (key > tmp->data.first) tmp = tmp->right;
			}

			throw std::out_of_range("no such key");
		}

		void rewrite_count(node_type *node, bool flag) {
			short k = -1;
			if (flag) k = 1;

			if (node != nullptr) {
				if ((node->left != nullptr) && (node->right != nullptr)) node->ref_count += 3 * k;
				else {
					if (node->state == LAST) {
						if ((node->left == nullptr) && (node->right == nullptr)) node->ref_count += 2 * k;
						else node->ref_count += 3 * k;
					}
					else {
						if ((node->left == nullptr) && (node->right == nullptr)) node->ref_count += 1 * k;
						else node->ref_count += 2 * k;
					}
				}
			}
		}

		void nodes_correction(node_type *x, node_type *y, node_type *z, bool flag) {
			if (z != nullptr) z->parent = x;
			else {
				y->ref_count += 1;
				x->ref_count -= 1;
			}

			y->parent = x->parent;
			x->parent = y;

			if (flag) {
				x->left = z;
				y->right = x;
			}
			else {
				x->right = z;
				y->left = x;
			}

			if (x->state == ROOT) {
				x->state = OCCUPIED;
				y->state = ROOT;
			}

			x->height = (1 + compare(node_height(x->left), node_height(x->right)));
			y->height = (1 + compare(node_height(y->left), node_height(y->right)));
		}

		node_type* right_rotation(node_type *node) {
			node_type *tmp_1 = node->left;
			node_type *tmp_2 = tmp_1->right;

			nodes_correction(node, tmp_1, tmp_2, true);

			return tmp_1;
		}

		node_type* left_rotation(node_type *node) {
			node_type *tmp_1 = node->right;
			node_type *tmp_2 = tmp_1->left;

			nodes_correction(node, tmp_1, tmp_2, false);

			return tmp_1;
		}

		node_type* push(node_type *node, node_type *p, const value_type &value) {
			if (node == nullptr) {
				node = new node_type(value);
				node->parent = p;
				node->ref_count += 1;
				p->ref_count += 1;

				if (((p->state == LAST) || (p->state == ROOT)) && (value.first > p->data.first)) {
					if (p->state == LAST) p->state = OCCUPIED;

					node->state = LAST;
					p->ref_count -= 1;
					node->ref_count += 1;
					this->sentinel.ptr = node;
				}

				this->size_++;

				return node;
			}

			if (node->state == FREE) {
				new (&node->data) value_type(std::move(value));
				node->state = ROOT;
				node->ref_count += 1;

				this->sentinel.ptr = node;
				this->sentinel.state = END;
				this->size_++;

				return node;
			}

			if (value.first < node->data.first) node->left = push(node->left, node, value);
			else if (value.first > node->data.first) node->right = push(node->right, node, value);
			else return node;

			node->height = (1 + compare(node_height(node->left), node_height(node->right)));
			int balance = get_balance(node);

			if ((balance < -1) && (value.first > node->right->data.first)) return left_rotation(node);
			if ((balance < -1) && (value.first < node->right->data.first)) {
				node->right = right_rotation(node->right);
				return left_rotation(node);
			}

			if ((balance > 1) && (value.first < node->left->data.first)) return right_rotation(node);
			if ((balance > 1) && (value.first > node->left->data.first)) {
				node->left = (left_rotation(node->left));
				return right_rotation(node);
			}

			return node;
		}

		void unlink(node_type *node, size_type count) {
			if (node->state != ROOT) node->parent->ref_count += 1;

			if (node->left != nullptr) node->left->ref_count += 1;
			if (node->right != nullptr) node->right->ref_count += 1;

			node->ref_count -= count;
			node->height = 1;
			node->state = REMOVED;
		}

		node_type* remove(node_type *node, const key_type &key) {
			if (node == nullptr) return node;

			if (node->data.first > key) node->left = remove(node->left, key);
			else if (node->data.first < key) node->right = remove(node->right, key);
			else {
				if ((node->left == nullptr) || (node->right == nullptr)) {
					node_type *tmp = node->left ? node->left : node->right;

					if (tmp == nullptr) {
						node->parent->ref_count -= 1;

						if (node->state == LAST) {
							node->parent->ref_count += 1;
							node->parent->state = LAST;
							node->ref_count -= 1;

							this->sentinel.ptr = node->parent;
						}

						if (node->ref_count > 1) unlink(node, 1);
						else tmp = node;

						node = nullptr;
					}
					else {
						if (node->state == LAST) {
							tmp->state = LAST;
							tmp->ref_count += 1;
							node->ref_count -= 1;

							this->sentinel.ptr = tmp;
						}

						tmp->parent = node->parent;

						if (node->state == ROOT) {
							if (node->ref_count > 1) unlink(node, 1);
							tmp->state = ROOT;
						}
						else if (node->ref_count > 2) unlink(node, 2);


						node = tmp;
						tmp = nullptr;
					}

					this->size_--;
				}
				else {
					node_type *tmp = find_min(node->right);
					bool flag = false;

					// relink parent
					if (tmp->right != nullptr) {
						if (tmp->parent->data.first > tmp->right->data.first) tmp->parent->left = tmp->right;
						else tmp->parent->right = tmp->right;

						tmp->right->parent = tmp->parent;
					}
					else {
						if (tmp->parent == node) {
							flag = true;
							tmp->parent->ref_count += 1;
						}

						if (tmp->parent->data.first > tmp->data.first) tmp->parent->left = nullptr;
						else tmp->parent->right = nullptr;
						tmp->parent->ref_count -= 1;
					}

					rewrite_count(tmp, false);
					tmp->parent = node->parent;
					tmp->left = node->left;
					tmp->right = node->right;
					if (node->right != nullptr) node->right->parent = tmp;
					if (node->left != nullptr) node->left->parent = tmp;
					tmp->height = node->height;
					rewrite_count(tmp, true);

					if (flag) {
						if (node->data.first > tmp->data.first) node->left = tmp;
						else node->right = tmp;
					}

					if (node->state == ROOT) {
						if (node->ref_count > 2) unlink(node, 2);
						tmp->state = ROOT;
					}
					else if (node->ref_count > 3) unlink(node, 3);


					node = tmp;
					tmp = nullptr;
				}
			}

			if ((node == nullptr) || (node->state == REMOVED)) return node;

			node->height = (1 + compare(node_height(node->left), node_height(node->right)));
			int balance = get_balance(node);

			if ((balance > 1) && (get_balance(node->left) >= 0)) return right_rotation(node);
			if ((balance > 1) && (get_balance(node->left) < 0)) {
				node->left = left_rotation(node->left);
				return right_rotation(node);
			}

			if ((balance < -1) && (get_balance(node->right) <= 0)) return left_rotation(node);
			if ((balance < -1) && (get_balance(node->right) > 0)) {
				node->right = right_rotation(node->right);
				return left_rotation(node);
			}

			return node;
		}

		node_type *root;
		iterator sentinel;
		size_type size_;
	};

}