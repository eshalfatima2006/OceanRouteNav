#ifndef QUEUE_H
#define QUEUE_H

#include <stdexcept>

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        
        Node(const T& value) : data(value), next(nullptr) {}
    };
    
    Node* frontNode;
    Node* rearNode;
    int size;
    
public:
    // Constructor
    Queue() : frontNode(nullptr), rearNode(nullptr), size(0) {}
    
    // Destructor
    ~Queue() {
        clear();
    }
    
    // Copy Constructor
    Queue(const Queue& other) : frontNode(nullptr), rearNode(nullptr), size(0) {
        Node* current = other.frontNode;
        while (current != nullptr) {
            enqueue(current->data);
            current = current->next;
        }
    }
    
    // Assignment Operator
    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            Node* current = other.frontNode;
            while (current != nullptr) {
                enqueue(current->data);
                current = current->next;
            }
        }
        return *this;
    }
    
    // Add element to rear of queue
    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        if (rearNode == nullptr) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        size++;
    }
    
    // Remove element from front of queue
    void dequeue() {
        if (frontNode == nullptr) {
            throw std::runtime_error("Cannot dequeue from empty queue");
        }
        Node* temp = frontNode;
        frontNode = frontNode->next;
        if (frontNode == nullptr) {
            rearNode = nullptr;
        }
        delete temp;
        size--;
    }
    
    // Get front element without removing
    T& front() {
        if (frontNode == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        return frontNode->data;
    }
    
    const T& front() const {
        if (frontNode == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        return frontNode->data;
    }
    
    // Get rear element without removing
    T& rear() {
        if (rearNode == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        return rearNode->data;
    }
    
    const T& rear() const {
        if (rearNode == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        return rearNode->data;
    }
    
    // Get size of queue
    int getSize() const {
        return size;
    }
    
    // Check if queue is empty
    bool isEmpty() const {
        return size == 0;
    }
    
    // Clear all elements
    void clear() {
        while (frontNode != nullptr) {
            Node* temp = frontNode;
            frontNode = frontNode->next;
            delete temp;
        }
        rearNode = nullptr;
        size = 0;
    }
    
    // Peek at element at position (0 = front)
    T& peek(int index) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range");
        }
        Node* current = frontNode;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
    
    const T& peek(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range");
        }
        Node* current = frontNode;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
};

#endif // QUEUE_H
