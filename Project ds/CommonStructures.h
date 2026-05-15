#pragma once
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <iostream>

class DockQueueSystem;

template <typename T>
struct DynArray {
    T* data;
    int capacity;
    int length;

    DynArray(int cap = 16) : data(nullptr), capacity(0), length(0) {
        if (cap > 0) {
            data = new T[cap];
            capacity = cap;
        }
    }

    // Copy constructor (deep copy)
    DynArray(const DynArray& other)
        : data(nullptr), capacity(0), length(other.length)
    {
        int alloc = other.capacity;
        if (alloc < other.length) alloc = other.length;
        if (alloc > 0) {
            data = new T[alloc];
            capacity = alloc;
            for (int i = 0; i < other.length; ++i) data[i] = other.data[i];
        }
    }

    // Copy assignment (deep copy)
    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;
        int alloc = other.capacity;
        if (alloc < other.length) alloc = other.length;
        T* newData = nullptr;
        if (alloc > 0) newData = new T[alloc];
        for (int i = 0; i < other.length; ++i) newData[i] = other.data[i];
        delete[] data;
        data = newData;
        capacity = alloc;
        length = other.length;
        return *this;
    }

    // Move constructor (transfer ownership)
    DynArray(DynArray&& other)
        : data(other.data), capacity(other.capacity), length(other.length)
    {
        other.data = nullptr;
        other.capacity = 0;
        other.length = 0;
    }

    // Move assignment (transfer ownership)
    DynArray& operator=(DynArray&& other) {
        if (this == &other) return *this;
        delete[] data;
        data = other.data;
        capacity = other.capacity;
        length = other.length;
        other.data = nullptr;
        other.capacity = 0;
        other.length = 0;
        return *this;
    }

    ~DynArray() { delete[] data; }

    void push(const T& value) {
        int required = length + 1;
        if (required > capacity) {
            int nc = (capacity <= 0) ? (required > 1 ? required * 2 : 1) : capacity * 2;
            if (nc < required) nc = required;
            T* nd = new T[nc];
            if (!nd) { std::cerr << "allocation failed\n"; std::exit(1); }
            for (int i = 0; i < length; ++i) nd[i] = data[i];
            delete[] data;
            data = nd;
            capacity = nc;
        }
        data[length] = value;
        ++length;
    }

    T& operator[](int i) { return data[i]; }
    const T& operator[](int i) const { return data[i]; }
    int size() const { return length; }
};

// RouteEdge
struct RouteEdge {
    int destPortID;
    std::string dateStr;
    std::string departStr;
    std::string arriveStr;
    long departTS;
    long arriveTS;
    int cost;
    std::string company;
    RouteEdge* next;
    RouteEdge() : destPortID(-1), departTS(0), arriveTS(0), cost(0), next(nullptr) {}
};

// Company-Specific Port Storage
struct CompanyShip {
    std::string shipName;
    std::string company;
    long arrivalTime;
    int processingTime;  // in minutes
    CompanyShip* next;

    CompanyShip(const std::string& name, const std::string& comp, long arrive, int process)
        : shipName(name), company(comp), arrivalTime(arrive),
        processingTime(process), next(nullptr) {
    }
};

struct CompanyQueue {
    std::string companyName;
    CompanyShip* front;
    CompanyShip* rear;
    int queueSize;
    int dockedShips;
    int maxDocks;

    CompanyQueue(const std::string& company, int docks = 2)
        : companyName(company), front(nullptr), rear(nullptr),
        queueSize(0), dockedShips(0), maxDocks(docks) {
    }

    ~CompanyQueue() {
        clear();
    }

    void clear() {
        while (front != nullptr) {
            CompanyShip* temp = front;
            front = front->next;
            delete temp;
        }
        rear = nullptr;
        queueSize = 0;
        dockedShips = 0;
    }

    void enqueue(const std::string& shipName, long arriveTime, int processTime) {
        CompanyShip* newShip = new CompanyShip(shipName, companyName, arriveTime, processTime);

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

    CompanyShip dequeue() {
        if (front == nullptr) {
            return CompanyShip("", "", 0, 0);
        }

        CompanyShip* temp = front;
        CompanyShip result = *temp;  // Copy data

        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }

        delete temp;
        queueSize--;
        return result;
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

// Company Storage at each Port

struct CompanyPortStorage {
    std::string companyName;
    CompanyQueue* shipQueue;      // Waiting ships
    DynArray<RouteEdge*> companyRoutes;  // Routes operated by this company FROM this port
    int allocatedDocks;           // Docks allocated to this company

    CompanyPortStorage(const std::string& name, int docks = 1)
        : companyName(name), allocatedDocks(docks) {
        shipQueue = new CompanyQueue(name, docks);
        companyRoutes = DynArray<RouteEdge*>(8);
    }

    ~CompanyPortStorage() {
        delete shipQueue;
       
    }
};

struct PortNode {
    int id;
    std::string name;
    int portCharge;
    float x, y;
    RouteEdge* edges;  // All edges from this port (regardless of company)

    DynArray<CompanyPortStorage*> companyStorages;

    DockQueueSystem* dockSystem;

    PortNode() : id(-1), portCharge(0), x(0), y(0), edges(nullptr),
        companyStorages(5), dockSystem(nullptr) {
    }

    ~PortNode() {
        for (int i = 0; i < companyStorages.size(); ++i) {
            delete companyStorages[i];
        }
    }

    // Get or create company storage
    CompanyPortStorage* getCompanyStorage(const std::string& company) {
        for (int i = 0; i < companyStorages.size(); ++i) {
            if (companyStorages[i]->companyName == company) {
                return companyStorages[i];
            }
        }
        
        // Allocate docks based on company importance
        int docks = 1;
        if (company == "MaerskLine" || company == "MSC" || company == "COSCO") docks = 2;

        CompanyPortStorage* newStorage = new CompanyPortStorage(company, docks);
        companyStorages.push(newStorage);
        return newStorage;
    }

    // Add a ship to the company's queue at this port
    void addShipToCompanyQueue(const std::string& shipName, const std::string& company,
        long arriveTime, int processTime) {
        CompanyPortStorage* storage = getCompanyStorage(company);
        storage->shipQueue->enqueue(shipName, arriveTime, processTime);
    }

    // Get total ships waiting at this port 
    int getTotalWaitingShips() const {
        int total = 0;
        for (int i = 0; i < companyStorages.size(); ++i) {
            total += companyStorages[i]->shipQueue->get_queue_size();
        }
        return total;
    }

    // Get total docked ships at this port
    int getTotalDockedShips() const {
        int total = 0;
        for (int i = 0; i < companyStorages.size(); ++i) {
            total += companyStorages[i]->shipQueue->get_docked_count();
        }
        return total;
    }

    // Get waiting ships for a specific company
    int getCompanyWaitingShips(const std::string& company) const {
        for (int i = 0; i < companyStorages.size(); ++i) {
            if (companyStorages[i]->companyName == company) {
                return companyStorages[i]->shipQueue->get_queue_size();
            }
        }
        return 0;
    }
};

// Simple chained hash map
struct HashEntry {
    std::string key;
    int val;
    HashEntry* next;  // plain pointer chain
    HashEntry(const std::string& k, int v) : key(k), val(v), next(nullptr) {}
};

struct SimpleHash {
    HashEntry** buckets;  // Array of pointers
    int nb;

    SimpleHash(int n = 1031) {
        nb = n ? n : 1;
        buckets = new HashEntry *[nb];
        for (int i = 0; i < nb; ++i) buckets[i] = nullptr;
    }

    ~SimpleHash() {
        for (int i = 0; i < nb; ++i) {
            HashEntry* current = buckets[i];
            while (current) {
                HashEntry* next = current->next;
                delete current;
                current = next;
            }
        }
        delete[] buckets;
    }

    void insert(const std::string& key, int val) {
        unsigned long idx = hashString(key);
        HashEntry* e = buckets[idx];
        while (e) {
            if (case_insensitive_equal(e->key.c_str(), key.c_str())) {
                e->val = val;
                return;
            }
            e = e->next;
        }
        HashEntry* ne = new HashEntry(key, val);
        ne->next = buckets[idx];
        buckets[idx] = ne;
    }

    int get(const std::string& key) const {
        unsigned long idx = hashString(key);
        HashEntry* e = buckets[idx];
        while (e) {
            if (case_insensitive_equal(e->key.c_str(), key.c_str())) return e->val;
            e = e->next;
        }
        return -1;
    }

private:
    unsigned long hashString(const std::string& s) const {
        unsigned long h = 5381;
        for (unsigned char c : s) h = ((h << 5) + h) + std::tolower(c);
        return h % static_cast<unsigned long>(nb);
    }

    static bool case_insensitive_equal(const char* a, const char* b) {
        while (*a && *b) {
            char ca = static_cast<char>(std::tolower(static_cast<unsigned char>(*a)));
            char cb = static_cast<char>(std::tolower(static_cast<unsigned char>(*b)));
            if (ca != cb) return false;
            ++a; ++b;
        }
        return std::tolower(static_cast<unsigned char>(*a)) == std::tolower(static_cast<unsigned char>(*b));
    }
};

// -------------------------------
// Graph
// -------------------------------
struct Graph {
    DynArray<PortNode*> portsArr;
    SimpleHash nameToID;
    int portCount;
    int routeCount;
    Graph() : portsArr(32), nameToID(1031), portCount(0), routeCount(0) {}
    ~Graph() {
        for (int i = 0; i < portsArr.size(); ++i) {
            PortNode* p = portsArr[i];
            if (!p) continue;
            RouteEdge* e = p->edges;
            while (e) {
                RouteEdge* nx = e->next;
                delete e;
                e = nx;
            }
            delete p;
        }
    }
    int add_port(const std::string& name) {
        int existing = nameToID.get(name);
        if (existing != -1) return existing;
        PortNode* p = new PortNode();
        p->id = portCount;
        p->name = name;
        p->portCharge = 0;
        p->edges = nullptr;
        portsArr.push(p);
        nameToID.insert(name, portCount);
        ++portCount;
        return p->id;
    }
    PortNode* get_port_by_id(int id) {
        if (id < 0 || id >= portsArr.size()) return nullptr;
        return portsArr[id];
    }
    int get_port_id(const std::string& name) const {
        return nameToID.get(name);
    }
    void add_edge(int originID, RouteEdge* edge) {
        if (originID < 0 || originID >= portsArr.size()) return;
        PortNode* p = portsArr[originID];
        if (!p) return;
        edge->next = p->edges;
        p->edges = edge;
        ++routeCount;
    }
};
