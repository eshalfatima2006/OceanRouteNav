#include "BookingSystem.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace std;

// Constructor to initialize the bookings array
BookingSystem::BookingSystem() : nextBookingID(1000), bookings(16) {
    // bookings is initialized with capacity 16
}

string BookingSystem::generateBookingID() {
    return "BR" + to_string(nextBookingID++);
}

DynArray<BookedLeg> BookingSystem::findFeasibleRoute(Graph& graph, int originID, int destID,
    const string& date, int maxHops) {
    DynArray<BookedLeg> route(8);  // Initialize with capacity

    // First try direct routes on the specified date
    PortNode* origin = graph.get_port_by_id(originID);
    if (!origin) return route;  // Return empty if origin not found

    RouteEdge* edge = origin->edges;

    while (edge) {
        if (edge->destPortID == destID && edge->dateStr == date) {
            BookedLeg leg;
            leg.fromPortID = originID;
            leg.toPortID = destID;
            leg.edgeUsed = edge;
            leg.departureTime = edge->departTS;
            leg.arrivalTime = edge->arriveTS;
            leg.legCost = static_cast<float>(edge->cost);
            leg.shippingCompany = edge->company;

            route.push(leg);
            return route;  // Found direct route
        }
        edge = edge->next;
    }

    // If no direct route, search for 1-stop connections
    edge = origin->edges;
    while (edge) {
        if (edge->dateStr == date) {
            PortNode* midPort = graph.get_port_by_id(edge->destPortID);
            if (midPort) {
                RouteEdge* secondLeg = midPort->edges;
                while (secondLeg) {
                    if (secondLeg->destPortID == destID &&
                        secondLeg->dateStr == date &&
                        edge->arriveTS + 120 <= secondLeg->departTS) { // 2-hour minimum transfer

                        // First leg
                        BookedLeg leg1;
                        leg1.fromPortID = originID;
                        leg1.toPortID = edge->destPortID;
                        leg1.edgeUsed = edge;
                        leg1.departureTime = edge->departTS;
                        leg1.arrivalTime = edge->arriveTS;
                        leg1.legCost = static_cast<float>(edge->cost);
                        leg1.shippingCompany = edge->company;

                        // Second leg
                        BookedLeg leg2;
                        leg2.fromPortID = edge->destPortID;
                        leg2.toPortID = destID;
                        leg2.edgeUsed = secondLeg;
                        leg2.departureTime = secondLeg->departTS;
                        leg2.arrivalTime = secondLeg->arriveTS;
                        leg2.legCost = static_cast<float>(secondLeg->cost);
                        leg2.shippingCompany = secondLeg->company;

                        route.push(leg1);
                        route.push(leg2);
                        return route;
                    }
                    secondLeg = secondLeg->next;
                }
            }
        }
        edge = edge->next;
    }

    return route;  // Empty if no route found
}

bool BookingSystem::checkTimeFeasibility(const BookedLeg& prev, RouteEdge* next,
    int minTransferMinutes) {
    return (next->departTS >= prev.arrivalTime + minTransferMinutes);
}

string BookingSystem::bookRoute(Graph& graph, int originID, int destID,
    const string& date, const string& shipName,
    const string& cargoType,
    DockQueueSystem** portDocks) {

    cout << "\n=== BOOKING ROUTE ===" << endl;
    PortNode* fromP = graph.get_port_by_id(originID);
    PortNode* toP = graph.get_port_by_id(destID);
    cout << "From: " << (fromP ? fromP->name : "?") << endl;
    cout << "To: " << (toP ? toP->name : "?") << endl;
    cout << "Date: " << date << endl;
    cout << "Ship: " << shipName << endl;
    cout << "Cargo: " << cargoType << endl;

    // Find feasible route
    DynArray<BookedLeg> routeArray = findFeasibleRoute(graph, originID, destID, date);

    if (routeArray.size() == 0) {
        cout << "? No feasible route found!" << endl;
        return "";  // No feasible route found
    }

    cout << "? Route found with " << routeArray.size() << " leg(s)" << endl;

    // Create booking
    ShipBooking booking;
    booking.bookingID = generateBookingID();
    booking.shipName = shipName;
    booking.cargoType = cargoType;
    booking.originID = originID;
    booking.destID = destID;
    booking.voyageDate = date;
    booking.confirmed = true;
    booking.bookingTime = time(nullptr);  // Use current time

    // Convert DynArray to MultiLegRoute
    for (int i = 0; i < routeArray.size(); ++i) {
        booking.route.addLeg(routeArray[i]);
    }

    // Calculate totals
    booking.totalCost = 0.0f;
    booking.totalTime = 0;

    BookedLeg* leg = booking.route.getFirst();
    BookedLeg* prevLeg = nullptr;
    int legIndex = 0;

    while (leg) {
        booking.totalCost += leg->legCost;

        long legTime = leg->arrivalTime - leg->departureTime;
        if (legTime < 0) legTime += 24 * 60;
        booking.totalTime += legTime;

        // Add port charges for layovers > 12 hours
        if (prevLeg) {
            long layover = leg->departureTime - prevLeg->arrivalTime;
            if (layover > 12 * 60) {
                PortNode* layoverPort = graph.get_port_by_id(leg->fromPortID);
                if (layoverPort) {
                    float charge = layoverPort->portCharge * (layover / (24.0f * 60.0f));
                    booking.totalCost += charge;
                    cout << "  + Port charge at " << layoverPort->name
                        << ": $" << charge << " (layover: " << layover / 60 << "h)" << endl;
                }
            }
        }

        cout << "  Leg " << (legIndex + 1) << ": "
            << graph.get_port_by_id(leg->fromPortID)->name << " -> "
            << graph.get_port_by_id(leg->toPortID)->name
            << " ($" << leg->legCost << ", " << legTime / 60 << "h)" << endl;

        prevLeg = leg;
        leg = booking.route.getNext();
        ++legIndex;
    }

    bookings.push(booking);

    cout << "?? Total Cost: $" << booking.totalCost << endl;
    cout << "??  Total Time: " << booking.totalTime / 60 << " hours" << endl;
    cout << "? Booking created! ID: " << booking.bookingID << endl;

    return booking.bookingID;
}

ShipBooking* BookingSystem::getBooking(const string& bookingID) {
    return findBookingByID(bookingID);
}

ShipBooking* BookingSystem::findBookingByID(const std::string& bookingID) {
    for (int i = 0; i < bookings.size(); ++i) {
        if (bookings[i].bookingID == bookingID) {
            return &bookings[i];
        }
    }
    return nullptr;
}

bool BookingSystem::cancelBooking(const string& bookingID) {
    for (int i = 0; i < bookings.size(); ++i) {
        if (bookings[i].bookingID == bookingID) {
            // Remove the booking by shifting array left
            for (int j = i; j < bookings.size() - 1; ++j) {
                bookings[j] = bookings[j + 1];
            }
            bookings.length--; // DynArray exposes length
            cout << "Booking " << bookingID << " cancelled." << endl;
            return true;
        }
    }
    return false;
}

void BookingSystem::updateBookingStatus(long currentTime) {
    // Update booking status based on simulation time
    for (int i = 0; i < bookings.size(); ++i) {
        if (bookings[i].confirmed) {
            long bookingAge = currentTime - bookings[i].bookingTime;
            long voyageDuration = bookings[i].totalTime / 60;  // Convert to hours

            // Mark as completed if voyage time has passed
            if (bookingAge > voyageDuration * 3600) {  // Convert hours to seconds
                // Could mark as completed here
            }
        }
    }
}

void BookingSystem::drawBookedRoutes(sf::RenderWindow& window, Graph& graph, sf::Font& font) {
    static long animationCounter = 0;
    animationCounter++;

    for (int i = 0; i < bookings.size(); ++i) {
        ShipBooking& booking = bookings[i];

        if (!booking.confirmed) continue;

        // Draw linked list visualization (layover connections)
        RouteLegNode* currentNode = booking.route.getHead();
        int legNum = 0;

        while (currentNode && currentNode->next) {
            BookedLeg& leg = currentNode->legData;
            BookedLeg& nextLeg = currentNode->next->legData;

            PortNode* fromPort = graph.get_port_by_id(leg.toPortID);
            PortNode* toPort = graph.get_port_by_id(nextLeg.fromPortID);

            if (fromPort && toPort) {
                sf::Vertex line[2];
                line[0].position = sf::Vector2f(fromPort->x, fromPort->y);
                line[1].position = sf::Vector2f(toPort->x, toPort->y);
                line[0].color = sf::Color(255, 100, 100, 150);  // Red for layovers
                line[1].color = sf::Color(255, 100, 100, 150);
                window.draw(line, 2, sf::Lines);

                sf::CircleShape node(5);
                node.setOrigin(5, 5);
                node.setPosition(fromPort->x, fromPort->y);
                node.setFillColor(sf::Color::Yellow);
                window.draw(node);

                sf::Text legText;
                legText.setFont(font);
                legText.setCharacterSize(10);
                legText.setString("L" + to_string(legNum + 1));
                legText.setFillColor(sf::Color::Black);
                legText.setPosition(fromPort->x - 5, fromPort->y - 15);
                window.draw(legText);
            }

            currentNode = currentNode->next;
            ++legNum;
        }

        // Draw the actual route legs (purple lines) with animation
        BookedLeg* legPtr = booking.route.getFirst();
        while (legPtr) {
            PortNode* fromPort = graph.get_port_by_id(legPtr->fromPortID);
            PortNode* toPort = graph.get_port_by_id(legPtr->toPortID);

            if (fromPort && toPort) {
                sf::Vector2f start(fromPort->x, fromPort->y);
                sf::Vector2f end(toPort->x, toPort->y);
                sf::Vector2f direction = end - start;
                float length = sqrt(direction.x * direction.x + direction.y * direction.y);

                if (length > 1.0f) {
                    sf::RectangleShape routeLine(sf::Vector2f(length, 4.0f));
                    routeLine.setPosition(start);
                    routeLine.setFillColor(sf::Color(180, 50, 220, 200));

                    float angle = atan2(direction.y, direction.x) * 180.0f / 3.14159f;
                    routeLine.setRotation(angle);
                    window.draw(routeLine);

                    // Animate ship movement along the route
                    float animationSpeed = 0.002f;
                    float progress = fmod(animationCounter * animationSpeed, 1.0f);

                    sf::Vector2f shipPos = start + direction * progress;

                    sf::CircleShape ship(7.0f);
                    ship.setOrigin(7.0f, 7.0f);
                    ship.setPosition(shipPos);
                    ship.setFillColor(sf::Color::Yellow);
                    ship.setOutlineColor(sf::Color::Black);
                    ship.setOutlineThickness(1.0f);
                    window.draw(ship);
                }
            }

            legPtr = booking.route.getNext();
        }
    }
}

DynArray<ShipBooking>& BookingSystem::getAllBookings() {
    return bookings;
}

// Feature #6: interactive editing
bool BookingSystem::insertLegIntoBooking(const std::string& bookingID, int position, const BookedLeg& leg) {
    ShipBooking* booking = findBookingByID(bookingID);
    if (!booking) return false;

    booking->route.insertLeg(position, leg);

    // Recalculate totals
    booking->totalCost = 0.0f;
    booking->totalTime = 0;
    BookedLeg* cur = booking->route.getFirst();
    while (cur) {
        booking->totalCost += cur->legCost;
        long legTime = cur->arrivalTime - cur->departureTime;
        if (legTime < 0) legTime += 24 * 60;
        booking->totalTime += legTime;
        cur = booking->route.getNext();
    }

    cout << "Leg inserted at position " << position << " in booking " << bookingID << endl;
    return true;
}

bool BookingSystem::removeLegFromBooking(const std::string& bookingID, int position) {
    ShipBooking* booking = findBookingByID(bookingID);
    if (!booking) return false;

    if (position < 0 || position >= booking->route.getLegCount()) {
        cout << "Invalid leg position: " << position << endl;
        return false;
    }

    booking->route.removeLeg(position);

    // Recalculate totals
    booking->totalCost = 0.0f;
    booking->totalTime = 0;
    BookedLeg* cur = booking->route.getFirst();
    while (cur) {
        booking->totalCost += cur->legCost;
        long legTime = cur->arrivalTime - cur->departureTime;
        if (legTime < 0) legTime += 24 * 60;
        booking->totalTime += legTime;
        cur = booking->route.getNext();
    }

    cout << "Leg removed from position " << position << " in booking " << bookingID << endl;
    return true;
}

bool BookingSystem::updateLegInBooking(const std::string& bookingID, int position, const BookedLeg& newLeg) {
    ShipBooking* booking = findBookingByID(bookingID);
    if (!booking) return false;
    BookedLeg* target = booking->route.getLeg(position);
    if (!target) return false;
    *target = newLeg;

    // Recalculate totals
    booking->totalCost = 0.0f;
    booking->totalTime = 0;
    BookedLeg* cur = booking->route.getFirst();
    while (cur) {
        booking->totalCost += cur->legCost;
        long legTime = cur->arrivalTime - cur->departureTime;
        if (legTime < 0) legTime += 24 * 60;
        booking->totalTime += legTime;
        cur = booking->route.getNext();
    }

    cout << "Leg updated at position " << position << " in booking " << bookingID << endl;
    return true;
}

void BookingSystem::printRouteDetails(const std::string& bookingID, Graph& graph) {
    ShipBooking* booking = findBookingByID(bookingID);
    if (!booking) {
        cout << "Booking " << bookingID << " not found." << endl;
        return;
    }

    cout << "\n=== Route Details for Booking " << bookingID << " ===" << endl;
    cout << "Ship: " << booking->shipName << ", Cargo: " << booking->cargoType << endl;
    cout << "Total Cost: $" << booking->totalCost << ", Total Time: "
        << booking->totalTime / 60 << " hours" << endl;
    cout << "Route (" << booking->route.getLegCount() << " legs):" << endl;

    BookedLeg* leg = booking->route.getFirst();
    int legNum = 1;

    while (leg) {
        PortNode* fromPort = graph.get_port_by_id(leg->fromPortID);
        PortNode* toPort = graph.get_port_by_id(leg->toPortID);

        cout << "  " << legNum++ << ". "
            << (fromPort ? fromPort->name : "?") << " -> "
            << (toPort ? toPort->name : "?") << endl;
        cout << "     Depart: " << leg->departureTime / 60 << ":"
            << (leg->departureTime % 60) << ", Arrive: "
            << leg->arrivalTime / 60 << ":" << (leg->arrivalTime % 60) << endl;
        cout << "     Company: " << leg->shippingCompany
            << ", Cost: $" << leg->legCost << endl;

        leg = booking->route.getNext();
    }
}
