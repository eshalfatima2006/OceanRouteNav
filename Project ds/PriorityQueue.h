#pragma once
#include "CommonStructures.h"

struct PQNode {
    int portID;
    int priority; // lower value = higher priority
    long arrivalTime;
};

class PriorityQueue {
private:
    PQNode* heap;
    int capacity;
    int size;

    void heapify_up(int index);
    void heapify_down(int index);
    void swap_nodes(int i, int j);

public:
    PriorityQueue(int initialCapacity = 100);
    ~PriorityQueue();

    void push(int portID, int priority, long arrivalTime = 0);
    PQNode pop();
    PQNode peek() const;
    bool is_empty() const;
    int get_size() const;
};
