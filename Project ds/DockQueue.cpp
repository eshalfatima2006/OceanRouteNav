#include "DockQueue.h"

DockQueueSystem::DockQueueSystem(int initialCapacity) {
    capacity = initialCapacity;
    companyQueues = new DockCompanyQueue * [capacity];
    companyNames = new string[capacity];
    companyCount = 0;

    // Initialize with major shipping companies from Routes.txt
    string defaultCompanies[] = {
        "Evergreen", "MSC", "ZIM", "COSCO", "MaerskLine",
        "PIL", "HapagLloyd", "YangMing", "CMA_CGM", "ONE"
    };

    for (int i = 0; i < 10 && i < capacity; i++) {
        addCompany(defaultCompanies[i], 2);
    }
}

DockQueueSystem::~DockQueueSystem() {
    for (int i = 0; i < companyCount; i++) {
        delete companyQueues[i];
    }
    delete[] companyQueues;
    delete[] companyNames;
}

void DockQueueSystem::addCompany(const string& company, int maxDocks) {
    if (companyCount >= capacity) return;

    // Check if company already exists
    if (findCompanyIndex(company) != -1) return;

    companyQueues[companyCount] = new DockCompanyQueue(company, maxDocks);
    companyNames[companyCount] = company;
    companyCount++;
}

void DockQueueSystem::enqueueShip(const string& shipName, const string& company,
    long arriveTime, int processTime) {
    int index = findCompanyIndex(company);

    // Create new company queue if it doesn't exist
    if (index == -1) {
        addCompany(company, 2);
        index = companyCount - 1;
    }

    companyQueues[index]->enqueue(shipName, arriveTime, processTime);

    cout << "[DOCK] Ship '" << shipName << "' (" << company << ") arrived. ";
    cout << "Company queue: " << companyQueues[index]->get_queue_size() << endl;
}

DockShip DockQueueSystem::dequeueShip(const string& company) {
    int index = findCompanyIndex(company);
    if (index == -1) return DockShip();

    return companyQueues[index]->dequeue();
}

bool DockQueueSystem::is_empty() const {
    for (int i = 0; i < companyCount; i++) {
        if (!companyQueues[i]->is_empty()) return false;
    }
    return true;
}

int DockQueueSystem::get_queue_size(const string& company) const {
    int index = findCompanyIndex(company);
    if (index == -1) return 0;
    return companyQueues[index]->get_queue_size();
}

int DockQueueSystem::get_docked_count(const string& company) const {
    int index = findCompanyIndex(company);
    if (index == -1) return 0;
    return companyQueues[index]->get_docked_count();
}

int DockQueueSystem::get_total_queue_size() const {
    int total = 0;
    for (int i = 0; i < companyCount; i++) {
        total += companyQueues[i]->get_queue_size();
    }
    return total;
}

int DockQueueSystem::get_total_docked_count() const {
    int total = 0;
    for (int i = 0; i < companyCount; i++) {
        total += companyQueues[i]->get_docked_count();
    }
    return total;
}

void DockQueueSystem::update(long currentTime) {
    // Every 60 time units, process ships from each company queue
    static long lastProcessTime = 0;

    if (currentTime - lastProcessTime > 60) {
        for (int i = 0; i < companyCount; i++) {
            if (!companyQueues[i]->is_empty() &&
                companyQueues[i]->get_docked_count() < companyQueues[i]->get_max_docks()) {

                companyQueues[i]->start_docking();
                cout << "[DOCK] " << companyNames[i] << " started docking a ship." << endl;
            }
        }
        lastProcessTime = currentTime;
    }

    // Every 120 time units, some ships finish docking
    if (currentTime % 120 == 0) {
        for (int i = 0; i < companyCount; i++) {
            if (companyQueues[i]->get_docked_count() > 0 && (rand() % 3 == 0)) {
                companyQueues[i]->finish_docking();
                cout << "[DOCK] " << companyNames[i] << " finished docking a ship." << endl;
            }
        }
    }
}

void DockQueueSystem::print_status() const {
    cout << "[DOCK SYSTEM] Total ships waiting: " << get_total_queue_size()
        << ", Total docked: " << get_total_docked_count() << endl;

    for (int i = 0; i < companyCount; i++) {
        if (companyQueues[i]->get_queue_size() > 0 || companyQueues[i]->get_docked_count() > 0) {
            cout << "  " << companyNames[i] << ": Q=" << companyQueues[i]->get_queue_size()
                << ", D=" << companyQueues[i]->get_docked_count()
                << "/" << companyQueues[i]->get_max_docks() << endl;
        }
    }
}