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

struct ShipBooking {
    std::string bookingID;
    std::string shipName;
    std::string cargoType;
    int originID;
    int destID;
    std::string voyageDate;
    DynArray<BookedLeg> legs;
    float totalCost;    // Changed from int to float
    long totalTime;     // in minutes
    bool confirmed;
    long bookingTime;   // simulation time when booked

    ShipBooking() : totalCost(0.0f), totalTime(0), confirmed(false), bookingTime(0) {}
};
class BookingSystem {
private:
    DynArray<ShipBooking> bookings;
    int nextBookingID;

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
};

#endif