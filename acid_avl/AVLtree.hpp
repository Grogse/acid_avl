#pragma once

#include <utility>
#include <memory>
#include <stdexcept>
#include <shared_mutex>

namespace AVLtree {
    std::size_t compare(std::size_t a, std::size_t b) {
        return (a > b) ? a : b;
    }

    enum states {
        REMOVED,
        DESTROY,
        BEGIN,
        VALID,
        ROOT,
        FREE,
        END,
    };

    class exception : std::exception {
        using base_class = std::exception;
        using base_class::base_class;
    };

    template<typename NODE>
    class SmartPointer {
    public:
        using node_type = NODE;

        template<typename KEY, typename DATA>
        friend class Node;

        template<typename KEY, typename DATA>
        friend class AVLiterator;

        template<typename KEY, typename DATA>
        friend class AVL;

        explicit SmartPointer(node_type *tmp) {
            if (tmp == nullptr) {
                this->core = nullptr;

                return;
            }

            this->core = new Core;
            this->core->ptr = tmp;
            this->core->ref_count = 1;
        }

        SmartPointer() {}

        SmartPointer(const SmartPointer &tmp) {
            if (tmp.core == nullptr) {
                this->core = nullptr;

                return;
            }

            tmp.core->ref_count++;
            this->core = tmp.core;
        }

        SmartPointer(SmartPointer &&tmp) {
            if (tmp.core == nullptr) {
                this->core = nullptr;

                return;
            }

            this->core = tmp.core;
            tmp.core = nullptr;
        }

        ~SmartPointer() {
            if (this->core == nullptr) return;

            if (this->core->ptr->state == states::DESTROY)
                this->core->ref_count = 1;

            this->core->ref_count--;
            if (this->core->ref_count == 0) {
                this->core->ptr->parent = nullptr;
                delete this->core;
            }
        }

        SmartPointer &operator=(const SmartPointer &tmp) {
            if (tmp.core == nullptr) {
                this->del();
                this->core = nullptr;

                return *this;
            }

            this->del();
            tmp.core->ref_count++;
            this->core = tmp.core;

            return *this;
        }

        SmartPointer &operator=(SmartPointer &&tmp) {
            if (tmp.core == nullptr) {
                this->del();
                this->core = nullptr;

                return *this;
            }

            this->del();
            this->core = tmp.core;
            tmp.core = nullptr;

            return *this;
        }

        SmartPointer &operator=(node_type *tmp) {
            this->del();

            if (tmp == nullptr) {
                this->core = nullptr;
                return *this;
            }

            this->core = new Core;
            this->core->ptr = tmp;
            this->core->ref_count = 1;

            return *this;
        }

        node_type &operator*() {
            if ((this->core == nullptr) || (this->core->ptr == nullptr)) {
                throw AVLtree::exception();
            }

            return *(this->core->ptr);
        }

        const node_type &operator*() const {
            if ((this->core == nullptr) || (this->core->ptr == nullptr)) {
                throw AVLtree::exception();
            }

            return *(this->core->ptr);
        }

        node_type* operator->() const {
            if (this->core == nullptr) return nullptr;
            return this->core->ptr;
        }

        node_type* get() const {
            if (this->core == nullptr) return nullptr;
            return this->core->ptr;
        }

        operator bool() const {
            return this->get() != nullptr;
        }

        template<typename U>
        bool operator==(const SmartPointer<U> &tmp) const {
            return static_cast<void*>(tmp.get()) == static_cast<void*>(this->get());
        }

        template<typename U>
        bool operator!=(const SmartPointer<U> &tmp) const {
            return !(*this == tmp);
        }

        std::size_t count_owners() const {
            if (this->get() == nullptr) return 0;
            else return this->core->ref_count;
        }

    private:
        class Core {
        public:
            Core() {
                this->ptr = nullptr;
                this->ref_count = 0;
            }

            ~Core() {
                delete this->ptr;
                this->ref_count = 0;
                this->ptr = nullptr;
            }

            node_type *ptr = nullptr;
            std::atomic<size_t> ref_count = 0;
        };

        Core* get_core() {
            return this->core;
        }

        void del() {
            if (this->core == nullptr) return;

            this->core->ref_count--;

            if (this->core->ref_count == 0) {
                this->core->ptr->parent = nullptr;
                delete this->core;
                this->core = nullptr;
            }
        }

        Core *core = nullptr;
    };

    template<typename KEY, typename DATA>
    class Node {
    protected:
        using key_type = KEY;
        using data_type = DATA;
        using state_for_node = states;
        using value_type = std::pair<const key_type, data_type>;
        using smart_ptr = SmartPointer<Node>;
        using size_type = std::size_t;

        template<typename KEY, typename DATA>
        friend class AVL;

        template<typename KEY, typename DATA>
        friend class AVLiterator;

        template<typename NODE>
        friend class SmartPointer;

        Node() : parent(), left(), right(), height(0), state(states::FREE) {}

        Node(const value_type &val) : Node() {
            new (&this->data) value_type(std::move(val)); //placement new
            this->state = states::VALID;
            this->height = 1;
        }

        ~Node() {
            if (this->state == states::DESTROY) {
                if (left) left->state = states::DESTROY;
                if (right) right->state = states::DESTROY;
            }
        }

        value_type data;

        smart_ptr parent;
        smart_ptr left;
        smart_ptr right;

        size_type height;
        state_for_node state;
    };

    template<typename KEY, typename DATA>
    class AVLiterator {
    public:
        using key_type = KEY;
        using data_type = DATA;
        using node_type = Node<key_type, data_type>;
        using state_for_iterator = states;
        using pointer = SmartPointer<node_type>;
        using reference = node_type&;
        using value_type = std::pair<const key_type, data_type>;

        template<typename KEY, typename DATA>
        friend class AVL;

        AVLiterator() noexcept : ptr(), end_(), state(FREE) {}

        AVLiterator(const pointer &smart_ptr, const pointer &end, std::shared_mutex *m) noexcept : ptr(smart_ptr), 
            end_(end), state(states::VALID), mutex(m) {}

        ~AVLiterator() {}

        key_type get_key() {
            std::shared_lock<std::shared_mutex> guard(*mutex);
            return this->ptr->data.first;
        }

        data_type get_value() {
            std::shared_lock<std::shared_mutex> guard(*mutex);
            return this->ptr->data.second;
        }

        void operator=(const pointer &smart_ptr) {
            if (smart_ptr) {
                this->ptr = smart_ptr;
                this->state = smart_ptr->state;
            }
        }

        void operator=(const AVLiterator &iterator) {
            this->ptr = iterator.ptr;
            this->mutex = iterator.mutex;
            this->state = iterator.state;
            this->end_ = iterator.end_;
        }

        pointer operator->() const {
            return this->ptr;
        }

        bool operator==(const AVLiterator &right) {
            std::shared_lock<std::shared_mutex> guard(*mutex);
            return this->ptr == right.ptr;
        }

        bool operator!=(const AVLiterator &right) {
            std::shared_lock<std::shared_mutex> guard(*mutex);
            return this->ptr != right.ptr;
        }

        // postfix ++
        AVLiterator operator++(int) {
            std::unique_lock<std::shared_mutex> guard(*mutex);
            AVLiterator tmp;
            tmp = *this;
            plus();

            return tmp;
        }

        // prefix ++
        AVLiterator& operator++() {
            std::unique_lock<std::shared_mutex> guard(*mutex);
            return plus();
        }

        // postfix --
        AVLiterator operator--(int) {
            std::unique_lock<std::shared_mutex> guard(*mutex);
            AVLiterator tmp;
            tmp = *this;
            minus();

            return tmp;
        }

        // prefix --
        AVLiterator& operator--() {
            std::unique_lock<std::shared_mutex> guard(*mutex);
            return minus();
        }

    protected:
        AVLiterator& plus() {
            if (this->ptr == this->end_->parent) {
                *this = this->end_;
            }

            if (this->ptr->state == states::REMOVED) {
                if (this->ptr->right) {
                    this->ptr = this->ptr->right;

                    return *this;
                }
                else {
                    if ((this->ptr->parent) && (this->ptr->data.first < this->ptr->parent->data.first)) {
                        this->ptr = this->ptr->parent;

                        return *this;
                    }
                    else {
                        pointer temp = this->ptr->parent;
                        while (temp->state != states::VALID) temp = temp->parent;
                        this->ptr = temp;

                        return *this;
                    }
                }
            }

            if (this->state == states::END) return *this;
            else {
                if (!(this->ptr->right)) {
                    if (this->ptr->data.first < this->ptr->parent->data.first) {
                        *this = this->ptr->parent;
                    }
                    else {
                        pointer temp = this->ptr->parent;
                        while (temp->data.first < this->ptr->data.first) temp = temp->parent;
                        *this = temp;
                    }
                }
                else *this = find_min(this->ptr->right);
            }

            return *this;
        }

        AVLiterator& minus() {
            if (this->ptr->state == states::REMOVED) {
                if (this->ptr->left) {
                    this->ptr = this->ptr->left;

                    return *this;
                }
                else {
                    if ((this->ptr->parent) && (this->ptr->data.first > this->ptr->parent->data.first)) {
                        this->ptr = this->ptr->parent;

                        return *this;
                    }
                    else {
                        pointer temp = this->ptr->parent;
                        while (temp->state != states::VALID) temp = temp->parent;
                        this->ptr = temp;

                        return *this;
                    }
                }
            }

            if (this->state == states::END) {
                *this = this->end_->parent;
                return *this;
            }

            if (this->state == states::BEGIN) return *this;
            else {
                if (!(this->ptr->left)) {
                    if (this->ptr->data.first > this->ptr->parent->data.first) {
                        *this = this->ptr->parent;
                    }
                    else {
                        pointer temp = this->ptr->parent;
                        while (temp->data.first > this->ptr->data.first) temp = temp->parent;
                        *this = temp;
                    }
                }
                else *this = find_max(this->ptr->left);
            }

            return *this;
        }

        pointer find_min(pointer &node) {
            pointer tmp = node;
            while (tmp->left) tmp = tmp->left;

            return tmp;
        }

        pointer find_max(pointer &node) {
            pointer tmp = node;
            while (tmp->right) tmp = tmp->right;

            return tmp;
        }

        void null_iterator() {
            this->ptr = nullptr;
            this->end_ = nullptr;
            this->state = states::FREE;
        }

        pointer ptr;
        pointer end_;
        state_for_iterator state;
        std::shared_mutex *mutex = nullptr;
    };

    template<typename KEY, typename DATA>
    class AVL {
    public:
        using key_type = KEY;
        using data_type = DATA;
        using node_type = Node<key_type, data_type>;
        using smart_ptr = SmartPointer<node_type>;
        using iterator = AVLiterator<key_type, data_type>;
        using value_type = std::pair<const key_type, data_type>;
        using size_type = std::size_t;

        AVL() : root(new node_type()), size_(0) {}

        ~AVL() {
            std::unique_lock<std::shared_mutex> guard(mutex);
            this->root->state = states::DESTROY;
        }

        void insert(const value_type &value) {
            std::unique_lock<std::shared_mutex> guard(mutex);
            if (this->root->state == states::FREE) push(this->root, this->root, value);
            else push(this->root->left, this->root->left, value);
        }

        void erase(const key_type &key) {
            std::unique_lock<std::shared_mutex> guard(mutex);
            remove(this->root->left, key);
        }

        iterator begin() {
            std::shared_lock<std::shared_mutex> guard(mutex);
            return this->begin_;
        }

        iterator end() {
            std::shared_lock<std::shared_mutex> guard(mutex);
            return this->end_;
        }

        data_type at(const key_type &key) {
            std::shared_lock<std::shared_mutex> guard(mutex);
            smart_ptr tmp = find(key);

            if (tmp) return tmp->data.second;
            throw std::out_of_range("key out of range");
        }

        size_type height() {
            std::shared_lock<std::shared_mutex> guard(mutex);
            return this->root->left->height;
        }

        size_type size() {
            std::shared_lock<std::shared_mutex> guard(mutex);
            return this->size_;
        }

        void print(smart_ptr &node) {
            std::shared_lock<std::shared_mutex> guard(mutex);
            if (node) {
                print(node->left);
                std::cout << node->data.first << " ";
                print(node->right);
            }
        }

    private:
        smart_ptr find(const key_type &key) {
            smart_ptr tmp = this->root->left;

            while (tmp) {
                if (key == tmp->data.first) return tmp;
                if (key < tmp->data.first) tmp = tmp->left;
                if (key > tmp->data.first) tmp = tmp->right;
            }

            return tmp;
        }

        size_type node_height(smart_ptr &node) {
            if (!(node)) return 0;
            return node->height;
        }

        int get_balance(smart_ptr &node) {
            if (!(node)) return 0;
            return static_cast<int>(node_height(node->left)) - static_cast<int>(node_height(node->right));
        }

        smart_ptr find_min(smart_ptr &node) {
            smart_ptr tmp = node;
            while (tmp->left) tmp = tmp->left;

            return tmp;
        }

        void init_tree(smart_ptr &node, const value_type &value) {
            node->left = new node_type(value);
            node->left->parent = node;
            node->left->state = states::BEGIN;
            this->sentinel = new node_type();
            this->sentinel->parent = node->left;
            this->sentinel->state = states::END;

            iterator tmp_1(node->left, this->sentinel, &mutex);
            tmp_1->state = states::BEGIN;
            iterator tmp_2(this->sentinel, this->sentinel, &mutex);
            tmp_2->state = states::END;

            this->begin_ = tmp_1;
            this->begin_.state = states::BEGIN;
            this->end_ = tmp_2;
            this->end_.state = states::END;
            this->size_++;
        }

        void link_node(smart_ptr &node, smart_ptr &p, const value_type &value) {
            node = new node_type(value);
            node->parent = p;

            if ((value.first < p->data.first) && (p->state == BEGIN)) {
                p->state = VALID;
                node->state = BEGIN;
                this->begin_ = node;
            }

            if ((value.first > p->data.first) && (p == this->sentinel->parent)) {
                this->sentinel->parent = node;
                this->sentinel->state = END;
            }

            this->size_++;
        }

        void nodes_correction(smart_ptr &x, smart_ptr &y, smart_ptr &z, bool flag) {
            smart_ptr tmp = x;
            if (z) z->parent = x;

            if (x->parent->state == states::ROOT) x->parent->left = y;

            y->parent = tmp->parent;
            tmp->parent = y;

            if (flag) {
                tmp->left = z;
                y->right = tmp;
            }
            else {
                tmp->right = z;
                y->left = tmp;
            }

            tmp->height = (1 + compare(node_height(tmp->left), node_height(tmp->right)));
            y->height = (1 + compare(node_height(y->left), node_height(y->right)));
        }

        smart_ptr& right_rotation(smart_ptr &node) {
            smart_ptr tmp_1 = node->left;
            smart_ptr tmp_2 = tmp_1->right;

            nodes_correction(node, tmp_1, tmp_2, true);

            return node;
        }

        smart_ptr& left_rotation(smart_ptr &node) {
            smart_ptr tmp_1 = node->right;
            smart_ptr tmp_2 = tmp_1->left;

            nodes_correction(node, tmp_1, tmp_2, false);

            return node;
        }

        smart_ptr& push(smart_ptr &node, smart_ptr &p, const value_type &value) {
            if ((p->state == states::FREE) || (!(node))) {
                if (p->state == states::FREE) {
                    init_tree(node, value);
                    node->state = states::ROOT;
                    return node;
                }

                link_node(node, p, value);
                return node;
            }

            if (value.first < node->data.first) {
                smart_ptr tmp = push(node->left, node, value);
                node->left = tmp;
            }
            else if (value.first > node->data.first) {
                smart_ptr tmp = push(node->right, node, value);
                node->right = tmp;
            }
            else return node;

            node->height = (1 + compare(node_height(node->left), node_height(node->right)));
            int balance = get_balance(node);

            if ((balance < -1) && (value.first > node->right->data.first))
                return left_rotation(node)->parent;
            if ((balance < -1) && (value.first < node->right->data.first)) {
                node->right = right_rotation(node->right)->parent;
                return left_rotation(node)->parent;
            }

            if ((balance > 1) && (value.first < node->left->data.first))
                return right_rotation(node)->parent;
            if ((balance > 1) && (value.first > node->left->data.first)) {
                node->left = left_rotation(node->left)->parent;
                return right_rotation(node)->parent;
            }

            return node;
        }

        smart_ptr& remove(smart_ptr &node, const key_type &key) {
            if (!(node)) return node;

            if (node->data.first > key) {
                smart_ptr tmp = remove(node->left, key);
                node->left = tmp;

            }
            else if (node->data.first < key) {
                smart_ptr tmp = remove(node->right, key);
                node->right = tmp;
            }
            else {
                if ((!(node->left)) || (!(node->right))) {
                    smart_ptr tmp = node->left ? node->left : node->right;

                    if (!(tmp)) {
                        if (node->state == states::BEGIN) {
                            if (node->parent->state != states::ROOT) {
                                node->parent->state = states::BEGIN;
                                node->state = states::VALID;
                                this->begin_ = node->parent;
                            }
                            else {
                                this->begin_.null_iterator();
                            }
                        }

                        if (this->sentinel->parent == node) {
                            if (this->size_ == 1) {
                                this->end_.null_iterator();
                                this->sentinel = nullptr;
                                this->root->state = states::FREE;
                            }
                            else {
                                this->sentinel->parent = node->parent;
                            }
                        }

                        if (node.get_core()->ref_count > 1) node->state = states::REMOVED;
                        if (node->parent->right == node) node->parent->right = nullptr;
                        else if (node->parent->left == node) node->parent->left = nullptr;

                        this->size_--;

                        return node;
                    }
                    else {
                        if (node->state == states::BEGIN) {
                            if (node->parent->state != states::ROOT) {
                                node->parent->state = states::BEGIN;
                                node->state = states::VALID;
                                this->begin_ = node->parent;
                            }
                            else {
                                this->begin_ = node->right;
                            }
                        }

                        if (this->sentinel->parent == node) this->sentinel->parent = tmp;

                        tmp->parent = node->parent;
                        if (node.get_core()->ref_count > 1) node->state = states::REMOVED;
                        if (node->parent->right == node) node->parent->right = tmp;
                        else if (node->parent->left == node) node->parent->left = tmp;

                        this->size_--;

                        return node;
                    }
                }
                else {
                    smart_ptr tmp = find_min(node->right);
                    if (tmp != node->right) node->right = remove(node->right, tmp->data.first);
                    smart_ptr node_ = tmp;

                    if (node_->state == states::REMOVED) node_->state = states::VALID;
                    if (this->sentinel->parent == node) this->sentinel->parent = node_;

                    if (tmp != node->right) {
                        node_->right = node->right;
                        if (node_->right) node_->right->parent = node_;
                        node_->left = node->left;
                        if (node_->left) node_->left->parent = node_;
                    }
                    else {
                        node_->left = node->left;
                        if (node_->left) node_->left->parent = node_;
                    }

                    node_->parent = node->parent;
                    if (node.get_core()->ref_count > 1) node->state = states::REMOVED;
                    if (node->parent->right == node) node->parent->right = node_;
                    else if (node->parent->left == node) node->parent->left = node_;

                    return node;
                }
            }

            node->height = (1 + compare(node_height(node->left), node_height(node->right)));
            int balance = get_balance(node);

            if ((balance > 1) && (get_balance(node->left) >= 0)) return right_rotation(node)->parent;
            if ((balance > 1) && (get_balance(node->left) < 0)) {
                node->left = left_rotation(node->left)->parent;
                return right_rotation(node)->parent;
            }

            if ((balance < -1) && (get_balance(node->right) <= 0)) return left_rotation(node)->parent;
            if ((balance < -1) && (get_balance(node->right) > 0)) {
                node->right = right_rotation(node->right)->parent;
                return left_rotation(node)->parent;
            }

            return node;
        }

        std::shared_mutex mutex;
        smart_ptr root;
        smart_ptr sentinel;
        iterator begin_;
        iterator end_;
        size_type size_;
    };
}
