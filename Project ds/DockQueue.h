#pragma once
#include <string>
#include <iostream>
#include "CommonStructures.h"

using namespace std;

struct DockShip {
    string shipName;
    string company;
    long arrivalTime;
    int processingTime;
    DockShip* next;

    DockShip() : shipName(""), company(""), arrivalTime(0), processingTime(0), next(nullptr) {}
    DockShip(const string& n, const string& c, long arrive, int process)
        : shipName(n), company(c), arrivalTime(arrive), processingTime(process), next(nullptr) {
    }
};

// Renamed to avoid colliding with the CompanyQueue defined in CommonStructures.h
struct DockCompanyQueue {
    string companyName;
    DockShip* front;
    DockShip* rear;
    int queueSize;
    int dockedShips;
    int maxDocks;

    DockCompanyQueue(const string& company, int docks = 2)
        : companyName(company), front(nullptr), rear(nullptr),
        queueSize(0), dockedShips(0), maxDocks(docks) {
    }

    ~DockCompanyQueue() {
        clear();
    }

    void clear() {
        while (front != nullptr) {
            DockShip* temp = front;
            front = front->next;
            delete temp;
        }
        rear = nullptr;
        queueSize = 0;
        dockedShips = 0;
    }

    void enqueue(const string& shipName, long arriveTime, int processTime) {
        DockShip* newShip = new DockShip(shipName, companyName, arriveTime, processTime);

        if (rear == nullptr) {
            front = newShip;
            rear = newShip;
        }
        else {
            rear->next = newShip;
            rear = newShip;
        }
        queueSize++;
    }

    DockShip dequeue() {
        if (front == nullptr) {
            return DockShip();
        }

        DockShip* temp = front;
        DockShip result = *temp;

        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }

        delete temp;
        queueSize--;
        return result;
    }

    DockShip* peek_front() const {
        return front;
    }

    bool is_empty() const {
        return front == nullptr;
    }

    int get_queue_size() const { return queueSize; }
    int get_docked_count() const { return dockedShips; }
    int get_max_docks() const { return maxDocks; }
    int get_available_docks() const { return maxDocks - dockedShips; }

    void start_docking() {
        if (!is_empty() && dockedShips < maxDocks) {
            dequeue();
            dockedShips++;
        }
    }

    void finish_docking() {
        if (dockedShips > 0) {
            dockedShips--;
        }
    }
};

// Main dock system with separate queues per company
class DockQueueSystem {
private:
    DockCompanyQueue** companyQueues;  // Array of company queues
    string* companyNames;
    int companyCount;
    int capacity;

    int findCompanyIndex(const string& company) const {
        for (int i = 0; i < companyCount; i++) {
            if (companyNames[i] == company) return i;
        }
        return -1;
    }

public:
    DockQueueSystem(int initialCapacity = 10);
    ~DockQueueSystem();

    // Add a company queue
    void addCompany(const string& company, int maxDocks = 2);

    // Ship operations
    void enqueueShip(const string& shipName, const string& company,
        long arriveTime, int processTime);
    DockShip dequeueShip(const string& company);

    // Status methods
    bool is_empty() const;
    int get_queue_size(const string& company) const;
    int get_docked_count(const string& company) const;
    int get_total_queue_size() const;
    int get_total_docked_count() const;

    // Returns the total queue size across all companies
    int get_queue_size() const {
        int total = 0;
        for (int i = 0; i < companyCount; ++i) {
            if (companyQueues[i]) {
                total += companyQueues[i]->get_queue_size();
            }
        }
        return total;
    }

    // Simulation
    void update(long currentTime);

    // Info
    void print_status() const;
};
