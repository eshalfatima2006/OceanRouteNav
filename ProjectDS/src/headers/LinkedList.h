#ifndef LINKEDLIST_H
#define LINKEDLIST_H

template<typename T>
struct Node {
    T data;
    Node* next;
    Node(T val) : data(val), next(nullptr) {}
};

template<typename T>
class LinkedList {
private:
    Node<T>* head;
    int count;

public:
    LinkedList();
    ~LinkedList();
    
    void insert(T value);
    bool deleteNode(T value);
    bool find(T value) const;
    int size() const;
    bool empty() const;
    void clear();
  
    class Iterator {
    private:
        Node<T>* current;
    
    public:
        Iterator(Node<T>* node);
        T operator*() const;
        Iterator& operator++();
        bool operator!=(const Iterator& other) const;
    };
    
    Iterator begin() const;
    Iterator end() const;
};

template<typename T>
LinkedList<T>::LinkedList() : head(nullptr), count(0) {}

template<typename T>
LinkedList<T>::~LinkedList() {
    clear();
}

template<typename T>
void LinkedList<T>::insert(T value) {
    Node<T>* newNode = new Node<T>(value);

    if (!head) {
        head = newNode;
    } else {
        Node<T>* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = newNode;
    }
    count++;
}

template<typename T>
bool LinkedList<T>::deleteNode(T value) {
    if (!head) return false;

    if (head->data == value) {
        Node<T>* temp = head;
        head = head->next;
        delete temp;
        count--;
        return true;
    }

    Node<T>* current = head;
    while (current->next && current->next->data != value)
        current = current->next;

    if (!current->next) return false;

    Node<T>* temp = current->next;
    current->next = temp->next;
    delete temp;
    count--;
    return true;
}

template<typename T>
bool LinkedList<T>::find(T value) const {
    Node<T>* current = head;
    while (current) {
        if (current->data == value) return true;
        current = current->next;
    }
    return false;
}

template<typename T>
int LinkedList<T>::size() const {
    return count;
}

template<typename T>
bool LinkedList<T>::empty() const {
    return head == nullptr;
}

template<typename T>
void LinkedList<T>::clear() {
    while (head) {
        Node<T>* temp = head;
        head = head->next;
        delete temp;
    }
    count = 0;
}

template<typename T>
LinkedList<T>::Iterator::Iterator(Node<T>* node) : current(node) {}

template<typename T>
T LinkedList<T>::Iterator::operator*() const {
    return current->data;
}

template<typename T>
typename LinkedList<T>::Iterator& LinkedList<T>::Iterator::operator++() {
    if (current) current = current->next;
    return *this;
}

template<typename T>
bool LinkedList<T>::Iterator::operator!=(const Iterator& other) const {
    return current != other.current;
}

template<typename T>
typename LinkedList<T>::Iterator LinkedList<T>::begin() const {
    return Iterator(head);
}

template<typename T>
typename LinkedList<T>::Iterator LinkedList<T>::end() const {
    return Iterator(nullptr);
}

#endif