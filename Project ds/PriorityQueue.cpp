#include "PriorityQueue.h"
#include <iostream>
using namespace std;

PriorityQueue::PriorityQueue(int initialCapacity) {
    capacity = initialCapacity;
    size = 0;
    heap = new PQNode[capacity];
}

PriorityQueue::~PriorityQueue() {
    delete[] heap;
}

void PriorityQueue::swap_nodes(int i, int j) {
    PQNode temp = heap[i];
    heap[i] = heap[j];
    heap[j] = temp;
}

void PriorityQueue::heapify_up(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (heap[index].priority >= heap[parent].priority) break;
        swap_nodes(index, parent);
        index = parent;
    }
}

void PriorityQueue::heapify_down(int index) {
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;

        if (left < size && heap[left].priority < heap[smallest].priority)
            smallest = left;
        if (right < size && heap[right].priority < heap[smallest].priority)
            smallest = right;

        if (smallest == index) break;
        swap_nodes(index, smallest);
        index = smallest;
    }
}

void PriorityQueue::push(int portID, int priority, long arrivalTime) {
    if (size >= capacity) {
        // Resize array
        int newCapacity = capacity * 2;
        PQNode* newHeap = new PQNode[newCapacity];
        // Copy only 'size' elements, which is safe
        for (int i = 0; i < size; i++) {
            newHeap[i] = heap[i];
        }
        delete[] heap;
        heap = newHeap;
        capacity = newCapacity;
    }

    if (size < capacity) { // Ensure we do not write out of bounds
        heap[size] = { portID, priority, arrivalTime };
        heapify_up(size);
        size++;
    } else {
        // This should never happen, but add a safety check
        cerr << "Error: PriorityQueue push out of bounds!" << endl;
    }
}

PQNode PriorityQueue::pop() {
    if (size == 0) {
        cerr << "PriorityQueue underflow!\n";
        return { -1, -1, 0 };
    }

    // EMERGENCY STOP: Add this safety check
    if (size > 10000) {
        cerr << "EMERGENCY STOP: Queue too large (" << size << " elements)" << endl;
        size = 0; // Clear the queue to break the loop
        return { -1, -1, 0 };
    }

    PQNode root = heap[0];
    heap[0] = heap[size - 1];
    size--;
    heapify_down(0);
    return root;
}
PQNode PriorityQueue::peek() const {
    if (size == 0) return { -1, -1, 0 };
    return heap[0];
}

bool PriorityQueue::is_empty() const {
    return size == 0;
}

int PriorityQueue::get_size() const {
    return size;
}
