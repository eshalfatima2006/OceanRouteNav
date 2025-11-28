#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <stdexcept>

// Min-Heap implementation (smallest element has highest priority)

class PriorityQueue {
private:
    struct Element {
        T data;
        double priority;
        
        Element() : data(T()), priority(0.0) {}
        Element(const T& d, double p) : data(d), priority(p) {}
    };
    
    Element* heap;
    int capacity;
    int size;
    
    // Helper function to get parent index
    int parent(int i) const {
        return (i - 1) / 2;
    }
    
    // Helper function to get left child index
    int leftChild(int i) const {
        return 2 * i + 1;
    }
    
    // Helper function to get right child index
    int rightChild(int i) const {
        return 2 * i + 2;
    }
    
    // Swap two elements
    void swap(int i, int j) {
        Element temp = heap[i];
        heap[i] = heap[j];
        heap[j] = temp;
    }
    
    // Move element up to maintain heap property
    void heapifyUp(int index) {
        while (index > 0 && heap[parent(index)].priority > heap[index].priority) {
            swap(index, parent(index));
            index = parent(index);
        }
    }
    
    // Move element down to maintain heap property
    void heapifyDown(int index) {
        int minIndex = index;
        int left = leftChild(index);
        int right = rightChild(index);
        
        if (left < size && heap[left].priority < heap[minIndex].priority) {
            minIndex = left;
        }
        
        if (right < size && heap[right].priority < heap[minIndex].priority) {
            minIndex = right;
        }
        
        if (index != minIndex) {
            swap(index, minIndex);
            heapifyDown(minIndex);
        }
    }
    
    // Resize the heap array
    void resize() {
        capacity *= 2;
        Element* newHeap = new Element[capacity];
        for (int i = 0; i < size; i++) {
            newHeap[i] = heap[i];
        }
        delete[] heap;
        heap = newHeap;
    }
    
public:
    // Constructor
    PriorityQueue(int initialCapacity = 16) 
        : capacity(initialCapacity), size(0) {
        heap = new Element[capacity];
    }
    
    // Destructor
    ~PriorityQueue() {
        delete[] heap;
    }
    
    // Copy Constructor
    PriorityQueue(const PriorityQueue& other) 
        : capacity(other.capacity), size(other.size) {
        heap = new Element[capacity];
        for (int i = 0; i < size; i++) {
            heap[i] = other.heap[i];
        }
    }
    
    // Assignment Operator
    PriorityQueue& operator=(const PriorityQueue& other) {
        if (this != &other) {
            delete[] heap;
            capacity = other.capacity;
            size = other.size;
            heap = new Element[capacity];
            for (int i = 0; i < size; i++) {
                heap[i] = other.heap[i];
            }
        }
        return *this;
    }
    
    // Insert element with priority
    void push(const T& value, double priority) {
        if (size == capacity) {
            resize();
        }
        
        heap[size] = Element(value, priority);
        heapifyUp(size);
        size++;
    }
    
    // Remove and return highest priority element (minimum priority value)
    void pop() {
        if (size == 0) {
            throw std::runtime_error("Cannot pop from empty priority queue");
        }
        
        heap[0] = heap[size - 1];
        size--;
        
        if (size > 0) {
            heapifyDown(0);
        }
    }
    
    // Get highest priority element without removing
    T& top() {
        if (size == 0) {
            throw std::runtime_error("Priority queue is empty");
        }
        return heap[0].data;
    }
    
    const T& top() const {
        if (size == 0) {
            throw std::runtime_error("Priority queue is empty");
        }
        return heap[0].data;
    }
    
    // Get priority of top element
    double topPriority() const {
        if (size == 0) {
            throw std::runtime_error("Priority queue is empty");
        }
        return heap[0].priority;
    }
    
    // Get size
    int getSize() const {
        return size;
    }
    
    // Check if empty
    bool isEmpty() const {
        return size == 0;
    }
    
    // Clear all elements
    void clear() {
        size = 0;
    }
    
    // Check if contains element (linear search)
    bool contains(const T& value) const {
        for (int i = 0; i < size; i++) {
            if (heap[i].data == value) {
                return true;
            }
        }
        return false;
    }
    
    // Update priority of an element (expensive operation - O(n))
    bool updatePriority(const T& value, double newPriority) {
        for (int i = 0; i < size; i++) {
            if (heap[i].data == value) {
                double oldPriority = heap[i].priority;
                heap[i].priority = newPriority;
                
                if (newPriority < oldPriority) {
                    heapifyUp(i);
                } else {
                    heapifyDown(i);
                }
                return true;
            }
        }
        return false;
    }
    
   
    bool getPriority(const T& value, double& priority) const {
        for (int i = 0; i < size; i++) {
            if (heap[i].data == value) {
                priority = heap[i].priority;
                return true;
            }
        }
        return false;
    }
};

#endif // PRIORITYQUEUE_H
