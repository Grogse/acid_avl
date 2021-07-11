#pragma once

#include <vector>
#include <atomic>
#include <shared_mutex>

namespace ACIDListMedium {

	enum states {
		REMOVED,
		BEGIN,
		VALID,
		END,
	};

	template <typename DATA>
	class Node {
	protected:
		using data_type = DATA;
		using state_for_node = states;
		using size_type = std::size_t;

		template <typename DATA>
		friend class Iterator;

		template <typename DATA>
		friend class List_medium;

		Node(states state) : data(), prev(nullptr), next(nullptr), state(state), ref_count(0) {}

		Node(data_type value) : data(value), prev(nullptr), next(nullptr), state(states::VALID), ref_count(0) {}

		void destroy() {
			std::unique_lock<std::shared_mutex> guard(mutex);

			this->ref_count--;
			if (this->ref_count == 0 && this->state == states::REMOVED) {
				guard.unlock();
				std::vector<Node*> stack;
				stack.push_back(this->prev);
				stack.push_back(this->next);

				delete this;

				while (!stack.empty()) {
					Node* unit = stack.back();
					stack.pop_back();

					if (unit) {
						std::unique_lock<std::shared_mutex> guard_u(unit->mutex);
						unit->ref_count--;

						if (unit->ref_count == 0 && unit->state == states::REMOVED) {
							guard_u.unlock();

							stack.push_back(unit->prev);
							stack.push_back(unit->next);
							delete unit;
						}
					}
				}
			}
		}

		data_type data;

		Node *prev;
		Node *next;

		std::atomic<states> state;
		std::atomic<size_type> ref_count;
		std::shared_mutex mutex;
	};

	template <typename DATA>
	class Iterator {
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using data_type = DATA;
		using reference = data_type&;
		using pointer = data_type*;
		using node = Node<DATA>;

		template <typename DATA>
		friend class List_medium;

		Iterator(const Iterator &other) noexcept {
			std::unique_lock<std::shared_mutex> guard(other.ptr->mutex);
			this->ptr = other.ptr;
			this->ptr->ref_count++;
		}

		Iterator(node* tmp) noexcept {
			this->ptr = tmp;
			this->ptr->ref_count++;
		}

		~Iterator() {
			this->ptr->destroy();
		}

		reference operator*() const {
			std::shared_lock<std::shared_mutex> guard(this->ptr->mutex);
			return this->ptr->data;
		}

		Iterator& operator=(const Iterator &other) {
			std::unique_lock<std::shared_mutex> guard(this->ptr->mutex);
			if (this->ptr == other.ptr) return *this;
			std::unique_lock<std::shared_mutex> guard_o(other.ptr->mutex);

			node *tmp = this->ptr;
			this->ptr = other.ptr;
			this->ptr->ref_count++;

			guard_o.unlock();
			guard.unlock();

			tmp->destroy();
			return *this;
		}

		Iterator& operator=(Iterator &&other) {
			std::unique_lock<std::shared_mutex> guard(this->ptr->mutex);
			if (this->ptr == other.ptr) return *this;
			std::unique_lock<std::shared_mutex> guard_o(other.ptr->mutex);

			node *tmp = this->ptr;
			this->ptr = other.ptr;
			this->ptr->ref_count++;

			guard_o.unlock();
			guard.unlock();

			tmp->destroy();
			return *this;
		}

		data_type get() {
			std::shared_lock<std::shared_mutex> guard(this->ptr->mutex);
			return this->ptr->data;
		}

		node* get_pointer() {
			return this->ptr;
		}

		void set(data_type value) {
			std::unique_lock<std::shared_mutex> guard(this->ptr->mutex);
			this->ptr->data = value;
		}

		Iterator& operator++() {
			plus();
			return *this;
		}

		Iterator operator++(int) {
			Iterator tmp = *this;
			plus();
			return tmp;
		}

		Iterator& operator--() {
			minus();
			return *this;
		}

		Iterator operator--(int) {
			Iterator tmp = *this;
			minus();
			return tmp;
		}

		bool operator==(const Iterator& other) {
			return other.ptr == this->ptr;
		}

		bool operator!=(const Iterator& other) {
			return other.ptr != this->ptr;
		}

	private:
		void plus() {
			if (this->ptr && this->ptr->state != states::END) {
				node* tmp;
				std::unique_lock<std::shared_mutex> guard(this->ptr->mutex);

				tmp = this->ptr;
				this->ptr = this->ptr->next;
				this->ptr->ref_count++;

				guard.unlock();
				tmp->destroy();
			}
		}

		void minus() {
			if (this->ptr && this->ptr->state != states::BEGIN) {
				node* tmp;
				std::unique_lock<std::shared_mutex> guard(this->ptr->mutex);

				tmp = this->ptr;
				this->ptr = this->ptr->prev;
				this->ptr->ref_count++;

				guard.unlock();
				tmp->destroy();
			}
		}

		node* ptr;
	};

	template <typename DATA>
	class List_medium {
	public:
		using size_type = std::size_t;
		using list_type = DATA;
		using data_type = Node<list_type>;
		using reference = list_type&;
		using const_reference = const list_type&;
		using iterator = Iterator<list_type>;

		List_medium(std::initializer_list<list_type> list) : List_medium() {
			for (auto it : list) push_back(it);
		}

		List_medium() {
			this->last = new data_type(states::END);
			this->root = new data_type(states::BEGIN);

			this->last->ref_count++;
			this->root->ref_count++;

			this->last->prev = this->root;
			this->root->next = this->last;
		}

		~List_medium() {
			data_type* tmp = this->root;

			while (tmp != this->last) {
				data_type* prev = tmp;
				tmp = tmp->next;
				delete prev;
			}

			delete tmp;
		}

		size_type size() {
			return this->size_;
		}

		iterator begin() {
			std::shared_lock<std::shared_mutex> guard(this->root->mutex);
			return iterator(this->root->next);
		}

		iterator end() {
			std::shared_lock<std::shared_mutex> guard(this->last->mutex);
			return iterator(this->last);
		}

		void push_front(list_type value) {
			std::unique_lock<std::shared_mutex> guard_w(this->root->mutex);
			data_type* tmp = this->root->next;
			std::unique_lock<std::shared_mutex> guard_r(tmp->mutex);

			data_type* new_node = new data_type(value);
			new_node->prev = this->root;
			new_node->next = tmp;
			new_node->ref_count++;
			new_node->ref_count++;


			tmp->prev = new_node;
			this->root->next = new_node;
			size_++;
		}

		void push_back(list_type value) {
			data_type* left;
			for (bool retry = true; retry;) {
				std::unique_lock<std::shared_mutex> guard(this->last->mutex);
				left = this->last->prev;
				left->ref_count++;
				guard.unlock();

				std::unique_lock<std::shared_mutex> guard_l(left->mutex);
				std::unique_lock<std::shared_mutex> guard_b(this->last->mutex);

				if (left->next == this->last && this->last->prev == left) {
					data_type* new_node = new data_type(value);

					new_node->prev = left;
					new_node->next = this->last;
					new_node->ref_count++;
					new_node->ref_count++;


					left->next = new_node;
					this->last->prev = new_node;
					this->size_++;

					retry = false;
				}

				guard_b.unlock();
				guard_l.unlock();
				left->destroy();
			}
		}

		void insert(iterator &it, list_type value) {
			data_type* left = it.ptr;

			if (left->state == states::END) push_back(value);
			else if (left->state == states::BEGIN) push_front(value);
			else {
				std::unique_lock<std::shared_mutex> guard_l(left->mutex);
				if (left->state == states::REMOVED) return;

				data_type* right = left->next;
				std::unique_lock<std::shared_mutex> guard_r(right->mutex);

				data_type* new_node = new data_type(value);

				new_node->ref_count++;
				new_node->ref_count++;
				new_node->prev = left;
				new_node->next = right;

				left->next = new_node;
				right->prev = new_node;

				this->size_++;
			}
		}

		iterator find(list_type value) {
			data_type* tmp = this->root;
			std::shared_lock<std::shared_mutex> guard(tmp->mutex);
			tmp = tmp->next;

			while (tmp != this->last && tmp->data != value) {
				guard = std::shared_lock<std::shared_mutex>(tmp->mutex);
				tmp = tmp->next;
			}

			return iterator(tmp);
		}

		void erase(iterator it) {
			data_type* node = it.ptr;

			if (this->size_ == 0 || node->state != states::VALID) return;

			for (bool retry = true; retry;) {

				std::shared_lock<std::shared_mutex> guard(node->mutex);
				if (node->state == states::REMOVED) return;

				data_type *left = node->prev;
				data_type *right = node->next;
				left->ref_count++;
				right->ref_count++;
				guard.unlock();

				std::unique_lock<std::shared_mutex> guard_l(left->mutex);
				std::shared_lock<std::shared_mutex> guard_c(node->mutex);
				std::unique_lock<std::shared_mutex> guard_r(right->mutex);

				if (left->next == node && right->prev == node) {
					node->state = states::REMOVED;

					node->ref_count--;
					node->ref_count--;

					left->next = right;
					right->prev = left;
					left->ref_count++;
					right->ref_count++;

					this->size_--;
					retry = false;
				}


				guard_r.unlock();
				guard_c.unlock();
				guard_l.unlock();
				left->destroy();
				right->destroy();
			}
		}

		void pop_back() {
			if (this->size_ != 0) {
				std::unique_lock<std::shared_mutex> guard(this->last->mutex);

				iterator it = iterator(this->last->prev);
				guard.unlock();
				erase(it);
			}
		}

	private:
		data_type* root;
		data_type* last;
		std::atomic<size_type> size_;
	};
}