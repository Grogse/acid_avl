#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <shared_mutex>

namespace ACIDListFine {

	enum states {
		REMOVED,
		BEGIN,
		VALID,
		END,
	};

	template <typename DATA>
	class Node {
	protected:
		template <typename DATA>
		friend class Iterator;

		template <typename DATA>
		friend class List_fine;

		template <typename DATA>
		friend class FreeList;

		using data_type = DATA;
		using state_for_node = states;
		using size_type = std::size_t;
		using cur_list = List_fine<DATA>;

		Node(states state, cur_list *clist) : list(clist), data(), prev(nullptr), next(nullptr), state(state), ref_count(0) {}

		Node(data_type value, cur_list *clist) :list(clist), data(value), prev(nullptr), next(nullptr), state(states::VALID),
			ref_count(0) {}

		void destroy() {
			std::shared_lock<std::shared_mutex> guard(this->list->freemutex);
			size_type ref = this->ref_count--;
			if (ref == 0) this->list->myfreelist->push(this);
		}

		cur_list *list;
		data_type data;

		Node *prev;
		Node *next;

		std::atomic<states> state;
		std::atomic<bool> already;
		std::atomic<size_type> ref_count;
		std::shared_mutex mutex;
	};

	template <typename DATA>
	class FreeNode {
	protected:
		using node = Node<DATA>;

		template<typename DATA>
		friend class FreeList;

		FreeNode(node* tmp) : ptr(tmp), next(nullptr) {}

		node *ptr;
		FreeNode *next;
	};

	template <typename DATA>
	class FreeList {
	protected:
		using list = List_fine<DATA>;
		using fnode = FreeNode<DATA>;
		using lnode = Node<DATA>;

		template<typename DATA>
		friend class List_fine;

		template<typename DATA>
		friend class Node;

		FreeList(list *tmp) : mylist(tmp) {
			this->cur_thread = std::thread(&FreeList::clear_list, this);
		}

		~FreeList() {
			this->clear = true;
			this->cur_thread.join();
		}

		void push(lnode* node) {
			fnode *pnode = new fnode(node);

			while (!this->main.compare_exchange_strong(pnode->next, pnode)) {
				pnode->next = this->main.load();
			}
		}

		void remove(fnode* prev, fnode* node) {
			prev->next = node->next;
			delete node;
		}

		void destroy_node(fnode* node) {
			lnode* left = node->ptr->prev;
			lnode* right = node->ptr->next;

			if (left) left->destroy();
			if (right) right->destroy();

			delete node->ptr;
			delete node;
		}

		void clear_list() {
			while (!this->clear || this->main.load()) {
				std::unique_lock<std::shared_mutex> guard(this->mylist->freemutex);
				fnode* tmp = this->main;
				guard.unlock();

				if (tmp) {
					fnode* prev = tmp;
					for (fnode* next_node = tmp; next_node;) {
						fnode* cur_node = next_node;
						next_node = next_node->next;

						if (cur_node->ptr->ref_count != 0 || cur_node->ptr->already == true) {
							remove(prev, cur_node);
						}
						else {
							cur_node->ptr->already = true;
							prev = cur_node;
						}
					}

					guard.lock();
					fnode* temp = this->main;

					if (tmp == temp) this->main = nullptr;

					guard.unlock();

					prev = temp;
					for (fnode* next_node = temp; next_node != tmp;) {
						fnode* cur_node = next_node;
						next_node = next_node->next;

						if (cur_node->ptr->already == true) remove(prev, cur_node);
						else prev = cur_node;
					}

					prev->next = nullptr;

					for (fnode* next_node = tmp; next_node;) {
						fnode* cur_node = next_node;
						next_node = next_node->next;
						destroy_node(cur_node);
					}
				}
				if (!this->clear) {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
			}
		}

		list *mylist;
		std::atomic<fnode*>	main;
		std::thread cur_thread;
		std::atomic<bool> clear;
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
		using list = List_fine<DATA>;

		template <typename DATA>
		friend class List_fine;

		template <typename DATA>
		friend class FreeList;

		Iterator(const Iterator &other) noexcept {
			std::unique_lock<std::shared_mutex> guard(other.ptr->mutex);
			this->ptr = other.ptr;
			this->ptr->ref_count++;
			this->mylist = other.mylist;
		}

		Iterator(node* value, list* list) noexcept {
			this->ptr = value;
			this->ptr->ref_count++;
			this->mylist = list;
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
			this->mylist = other.mylist;

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
			this->mylist = other.mylist;

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
				std::shared_lock<std::shared_mutex> guard(this->mylist->freemutex);

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
				std::shared_lock<std::shared_mutex> guard(this->mylist->freemutex);

				tmp = this->ptr;
				this->ptr = this->ptr->prev;
				this->ptr->ref_count++;

				guard.unlock();
				tmp->destroy();
			}
		}

		node *ptr;
		list *mylist;
	};

	template <typename DATA>
	class List_fine {
	public:
		using size_type = std::size_t;
		using list_type = DATA;
		using data_type = Node<list_type>;
		using reference = list_type&;
		using const_reference = const list_type&;
		using iterator = Iterator<list_type>;
		using freelist = FreeList<DATA>;

		template <typename DATA>
		friend class FreeList;

		template <typename DATA>
		friend class Node;

		template <typename DATA>
		friend class Iterator;

		List_fine(std::initializer_list<list_type> list) : List_fine() {
			for (auto it : list) push_back(it);
		}

		List_fine() {
			this->last = new data_type(states::END, this);
			this->root = new data_type(states::BEGIN, this);
			this->myfreelist = new freelist(this);

			this->last->ref_count++;
			this->root->ref_count++;

			this->last->prev = this->root;
			this->root->next = this->last;
		}

		~List_fine() {
			delete this->myfreelist;
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
			return iterator(this->root->next, this);
		}

		iterator end() {
			std::shared_lock<std::shared_mutex> guard(this->last->mutex);
			return iterator(this->last, this);
		}

		void push_front(list_type value) {
			std::unique_lock<std::shared_mutex> guard_w(this->root->mutex);
			data_type* tmp = this->root->next;
			std::unique_lock<std::shared_mutex> guard_r(tmp->mutex);

			data_type* new_node = new data_type(value, this);
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
					data_type* new_node = new data_type(value, this);

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

				data_type* new_node = new data_type(value, this);

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

			return iterator(tmp, this);
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
		freelist *myfreelist;
		std::shared_mutex freemutex;
	};
}