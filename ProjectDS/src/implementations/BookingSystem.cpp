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
    cout << "From: " << graph.get_port_by_id(originID)->name << endl;
    cout << "To: " << graph.get_port_by_id(destID)->name << endl;
    cout << "Date: " << date << endl;
    cout << "Ship: " << shipName << endl;
    cout << "Cargo: " << cargoType << endl;

    // Find feasible route
    DynArray<BookedLeg> route = findFeasibleRoute(graph, originID, destID, date);

    if (route.size() == 0) {
        cout << "? No feasible route found!" << endl;
        return "";  // No feasible route found
    }

    cout << "? Route found with " << route.size() << " leg(s)" << endl;

    // Create booking
    ShipBooking booking;
    booking.bookingID = generateBookingID();
    booking.shipName = shipName;
    booking.cargoType = cargoType;
    booking.originID = originID;
    booking.destID = destID;
    booking.voyageDate = date;
    booking.legs = route;
    booking.confirmed = true;
    booking.bookingTime = time(nullptr);  // Use current time

    // Calculate totals
    booking.totalCost = 0.0f;
    booking.totalTime = 0.0f;

    for (size_t i = 0; i < route.size(); ++i) {
        booking.totalCost += route[i].legCost;
        if (i > 0) {
            // Add port charges for layovers > 12 hours
            long layover = route[i].departureTime - route[i - 1].arrivalTime;
            if (layover > 12 * 60) {
                PortNode* layoverPort = graph.get_port_by_id(route[i].fromPortID);
                if (layoverPort) {
                    float charge = layoverPort->portCharge * (layover / (24.0f * 60.0f));
                    booking.totalCost += charge;
                    cout << "  + Port charge at " << layoverPort->name
                        << ": $" << charge << " (layover: " << layover / 60 << "h)" << endl;
                }
            }
        }

        long legTime = route[i].arrivalTime - route[i].departureTime;
        if (legTime < 0) legTime += 24 * 60;  // Handle overnight
        booking.totalTime += legTime;

        cout << "  Leg " << (i + 1) << ": "
            << graph.get_port_by_id(route[i].fromPortID)->name << " -> "
            << graph.get_port_by_id(route[i].toPortID)->name
            << " ($" << route[i].legCost << ", " << legTime / 60 << "h)" << endl;
    }

    bookings.push(booking);

    cout << "?? Total Cost: $" << booking.totalCost << endl;
    cout << "??  Total Time: " << booking.totalTime / 60 << " hours" << endl;
    cout << "? Booking created! ID: " << booking.bookingID << endl;

    return booking.bookingID;
}

ShipBooking* BookingSystem::getBooking(const string& bookingID) {
    for (size_t i = 0; i < bookings.size(); ++i) {
        if (bookings[i].bookingID == bookingID) {
            return &bookings[i];
        }
    }
    return nullptr;
}

bool BookingSystem::cancelBooking(const string& bookingID) {
    for (size_t i = 0; i < bookings.size(); ++i) {
        if (bookings[i].bookingID == bookingID) {
            // Remove the booking
            for (size_t j = i; j < bookings.size() - 1; ++j) {
                bookings[j] = bookings[j + 1];
            }
            bookings.length--;
            cout << "Booking " << bookingID << " cancelled." << endl;
            return true;
        }
    }
    return false;
}

void BookingSystem::updateBookingStatus(long currentTime) {
    // Update booking status based on simulation time
    for (size_t i = 0; i < bookings.size(); ++i) {
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

    for (size_t i = 0; i < bookings.size(); ++i) {
        ShipBooking& booking = bookings[i];

        if (!booking.confirmed) continue;

        // Draw each leg of the booked route
        for (size_t j = 0; j < booking.legs.size(); ++j) {
            BookedLeg& leg = booking.legs[j];
            PortNode* fromPort = graph.get_port_by_id(leg.fromPortID);
            PortNode* toPort = graph.get_port_by_id(leg.toPortID);

            if (!fromPort || !toPort) continue;

            // Calculate line properties
            sf::Vector2f start(fromPort->x, fromPort->y);
            sf::Vector2f end(toPort->x, toPort->y);
            sf::Vector2f direction = end - start;
            float length = sqrt(direction.x * direction.x + direction.y * direction.y);

            if (length < 1.0f) continue;  // Skip very short lines

            // Draw the booked route line (purple)
            sf::RectangleShape routeLine(sf::Vector2f(length, 4.0f));
            routeLine.setPosition(start);
            routeLine.setFillColor(sf::Color(180, 50, 220, 200)); // Purple

            float angle = atan2(direction.y, direction.x) * 180.0f / 3.14159f;
            routeLine.setRotation(angle);
            window.draw(routeLine);

            // Animate ship movement along the route
            float animationSpeed = 0.002f;
            float progress = fmod(animationCounter * animationSpeed, 1.0f);

            // Calculate ship position along the route
            sf::Vector2f shipPos = start + direction * progress;

            // Draw animated ship
            sf::CircleShape ship(7.0f);
            ship.setOrigin(7.0f, 7.0f);
            ship.setPosition(shipPos);
            ship.setFillColor(sf::Color::Yellow);
            ship.setOutlineColor(sf::Color::Black);
            ship.setOutlineThickness(1.0f);
            window.draw(ship);

            // Draw direction triangle
            sf::ConvexShape directionTriangle(3);
            directionTriangle.setPoint(0, sf::Vector2f(0, -5));
            directionTriangle.setPoint(1, sf::Vector2f(10, 0));
            directionTriangle.setPoint(2, sf::Vector2f(0, 5));
            directionTriangle.setFillColor(sf::Color::Red);
            directionTriangle.setPosition(shipPos);
            directionTriangle.setRotation(angle);
            window.draw(directionTriangle);

            // Draw booking info near the ship (only for first leg to avoid clutter)
            if (j == 0) {
                sf::Text infoText;
                infoText.setFont(font);
                infoText.setCharacterSize(11);
                infoText.setString(booking.shipName + "\n" + booking.cargoType);
                infoText.setFillColor(sf::Color::Black);
                infoText.setPosition(shipPos + sf::Vector2f(15.0f, -20.0f));

                // Background for readability
                sf::FloatRect textBounds = infoText.getLocalBounds();
                sf::RectangleShape textBg(sf::Vector2f(textBounds.width + 6, textBounds.height + 6));
                textBg.setFillColor(sf::Color(255, 255, 255, 200));
                textBg.setPosition(infoText.getPosition() - sf::Vector2f(3, 3));
                window.draw(textBg);

                window.draw(infoText);
            }
        }
    }
}