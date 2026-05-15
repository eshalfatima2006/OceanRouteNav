#ifndef BOOKINGSYSTEM_H
#define BOOKINGSYSTEM_H

#include "CommonStructures.h"
#include <SFML/Graphics.hpp>
#include <string>

// Forward declaration for DockQueueSystem if needed
class DockQueueSystem;

struct BookedLeg {
    int fromPortID;
    int toPortID;
    RouteEdge* edgeUsed;
    long departureTime;
    long arrivalTime;
    float legCost;  // Changed from int to float
    std::string shippingCompany;

    BookedLeg() : fromPortID(-1), toPortID(-1), edgeUsed(nullptr),
        departureTime(0), arrivalTime(0), legCost(0.0f) {
    }
};

// Doubly-linked list node and container for multi-leg routes (Feature #6)
struct RouteLegNode {
    BookedLeg legData;
    RouteLegNode* next;
    RouteLegNode* prev;

    RouteLegNode(const BookedLeg& leg) : legData(leg), next(nullptr), prev(nullptr) {}
};

class MultiLegRoute {
private:
    RouteLegNode* head;
    RouteLegNode* tail;
    RouteLegNode* iterPtr; // for getFirst/getNext iteration
    int legCount;

public:
    MultiLegRoute() : head(nullptr), tail(nullptr), iterPtr(nullptr), legCount(0) {}
    ~MultiLegRoute() { clear(); }

    // Deep-copy constructor
    MultiLegRoute(const MultiLegRoute& other) : head(nullptr), tail(nullptr), iterPtr(nullptr), legCount(0) {
        RouteLegNode* cur = other.head;
        while (cur) {
            addLeg(cur->legData);
            cur = cur->next;
        }
    }

    // Deep-copy assignment
    MultiLegRoute& operator=(const MultiLegRoute& other) {
        if (this == &other) return *this;
        clear();
        RouteLegNode* cur = other.head;
        while (cur) {
            addLeg(cur->legData);
            cur = cur->next;
        }
        return *this;
    }

    void clear() {
        RouteLegNode* cur = head;
        while (cur) {
            RouteLegNode* nx = cur->next;
            delete cur;
            cur = nx;
        }
        head = tail = iterPtr = nullptr;
        legCount = 0;
    }

    void addLeg(const BookedLeg& leg) {
        RouteLegNode* n = new RouteLegNode(leg);
        if (!head) {
            head = tail = n;
        } else {
            tail->next = n;
            n->prev = tail;
            tail = n;
        }
        ++legCount;
    }

    void insertLeg(int position, const BookedLeg& leg) {
        if (position < 0 || position > legCount) return;
        if (position == legCount) { addLeg(leg); return; }
        if (position == 0) {
            RouteLegNode* n = new RouteLegNode(leg);
            n->next = head;
            if (head) head->prev = n;
            head = n;
            if (!tail) tail = head;
            ++legCount;
            return;
        }
        RouteLegNode* cur = head;
        for (int i = 0; i < position; ++i) cur = cur->next;
        RouteLegNode* n = new RouteLegNode(leg);
        n->next = cur;
        n->prev = cur->prev;
        if (cur->prev) cur->prev->next = n;
        cur->prev = n;
        ++legCount;
    }

    void removeLeg(int position) {
        if (position < 0 || position >= legCount) return;
        RouteLegNode* cur = head;
        for (int i = 0; i < position; ++i) cur = cur->next;
        if (!cur) return;
        if (cur->prev) cur->prev->next = cur->next; else head = cur->next;
        if (cur->next) cur->next->prev = cur->prev; else tail = cur->prev;
        delete cur;
        --legCount;
    }

    BookedLeg* getLeg(int position) {
        if (position < 0 || position >= legCount) return nullptr;
        RouteLegNode* cur = head;
        for (int i = 0; i < position; ++i) cur = cur->next;
        return cur ? &cur->legData : nullptr;
    }

    int getLegCount() const { return legCount; }

    // Iterator API used by existing code
    BookedLeg* getFirst() {
        iterPtr = head;
        return iterPtr ? &iterPtr->legData : nullptr;
    }
    BookedLeg* getNext() {
        if (!iterPtr) return nullptr;
        iterPtr = iterPtr->next;
        return iterPtr ? &iterPtr->legData : nullptr;
    }

    // For visualization
    RouteLegNode* getHead() const { return head; }
};

struct BookedLeg; // keep forward for older references

struct ShipBooking {
    std::string bookingID;
    std::string shipName;
    std::string cargoType;
    int originID;
    int destID;
    std::string voyageDate;
    MultiLegRoute route;  // changed from DynArray<BookedLeg>
    float totalCost;    // Changed from int to float
    long totalTime;     // in minutes
    bool confirmed;
    long bookingTime;   // simulation time when booked

    ShipBooking() : originID(-1), destID(-1), totalCost(0.0f), totalTime(0), confirmed(false), bookingTime(0) {}
};

class BookingSystem {
private:
    DynArray<ShipBooking> bookings;
    int nextBookingID;

    ShipBooking* findBookingByID(const std::string& bookingID);

public:
    BookingSystem();

    std::string bookRoute(Graph& graph, int originID, int destID,
        const std::string& date, const std::string& shipName,
        const std::string& cargoType,
        DockQueueSystem** portDocks = nullptr);

    bool cancelBooking(const std::string& bookingID);
    ShipBooking* getBooking(const std::string& bookingID);
    DynArray<ShipBooking>& getAllBookings();

    void updateBookingStatus(long currentTime);
    void drawBookedRoutes(sf::RenderWindow& window, Graph& graph, sf::Font& font);

private:
    DynArray<BookedLeg> findFeasibleRoute(Graph& graph, int originID, int destID,
        const std::string& date, int maxHops = 4);

    bool checkTimeFeasibility(const BookedLeg& prev, RouteEdge* next,
        int minTransferMinutes = 120);

    std::string generateBookingID();

public:
  
    bool insertLegIntoBooking(const std::string& bookingID, int position,
                              const BookedLeg& leg);
    bool removeLegFromBooking(const std::string& bookingID, int position);
    bool updateLegInBooking(const std::string& bookingID, int position,
                            const BookedLeg& newLeg);
    void printRouteDetails(const std::string& bookingID, Graph& graph);
};

#endif
