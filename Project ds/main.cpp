#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdlib>

// Core system headers
#include "CommonStructures.h"
#include "PriorityQueue.h"
#include "BookingSystem.h"
#include "PathFinder.h"
#include "DockQueue.h"

// GUI and utility headers
#include "MathUtilities.h"
#include "RouteUtilities.h"
#include "DataLoader.h"
#include "PortLayout.h"
#include "UserPreferences.h"
#include "GUIRenderer.h"
#include "GUIInputHandler.h"

using namespace std;

int main() {
    // Seed random for Q key
    srand((unsigned)time(nullptr));

    Graph graph;

    bool ok1 = load_port_charges(graph, "PortCharges.txt");
    bool ok2 = load_routes(graph, "Routes.txt");
    if (!ok2) {
        cerr << "Routes.txt not loaded. Exiting.\n";
        return 1;
    }

    BookingSystem bookingSystem;
    UserPreferences userPrefs;
    PathFinder pathFinder(&graph);

    // Wire PathFinder edge filter to respect userPrefs
    static EdgeFilterCtx edgeFilterCtx;
    edgeFilterCtx.prefs = &userPrefs;
    edgeFilterCtx.graph = &graph;
    pathFinder.set_edge_filter(user_pref_edge_filter, &edgeFilterCtx);

    cout << "\n=== INITIALIZING DOCKING SYSTEM ===" << endl;
    for (int i = 0; i < graph.portsArr.size(); ++i) {
        PortNode* port = graph.portsArr[i];
        int docks = 2;
        if (port->name == "Singapore" || port->name == "Rotterdam" ||
            port->name == "Shanghai" || port->name == "Dubai") {
            docks = 4;
        }
        else if (port->name == "HongKong" || port->name == "Busan" ||
            port->name == "LosAngeles" || port->name == "NewYork") {
            docks = 3;
        }
        port->dockSystem = new DockQueueSystem();
        cout << "Port " << port->name << ": " << docks << " docking slots" << endl;
    }

    const int WINW = 1200, WINH = 800;
    assign_port_positions(graph, WINW, WINH);

    sf::RenderWindow window(sf::VideoMode(WINW, WINH), "OceanRoute Nav - Complete System");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("Arial.ttf")) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            cerr << "Failed to load fonts. Labels may not render.\n";
        }
    }

    sf::Texture mapTex;
    bool haveMap = false;
    if (mapTex.loadFromFile("world_map.png")) haveMap = true;

    string originQuery = "";
    string destQuery = "";
    PortNode* hoverPort = nullptr;
    PortNode* hoverEdgeFrom = nullptr;
    RouteEdge* hoverEdge = nullptr;

    // Animation / visualization state
    float animationProgress = 0.0f;
    bool animatingPath = false;
    sf::Clock animationClock;
    string currentAlgorithmName = "";
    int currentExploredCount = 0;
    int currentPathLength = 0;

    bool algorithmRunning = false;
    bool useCostOptimization = true;
    PathResult currentPathResult;
    DynArray<int> exploredPorts;
    sf::Clock algorithmClock;
    float algorithmTime = 0.0f;
    long simulationTime = 0;

    // Filtering and user preferences state
    bool filterByCompany = false;
    string filterCompany = "";
    DynArray<int> filteredPorts(20);

    // Dock status overlay toggle
    bool showDockPanel = false;

    // Click-to-book state
    ClickBookingState uiState;

    cout << "=== OceanRoute Nav - Complete System ===" << endl;
    cout << "Enter origin port name: ";
    getline(cin, originQuery);
    cout << "Enter destination port name: ";
    getline(cin, destQuery);

    int originID = graph.get_port_id(originQuery);
    int destID = graph.get_port_id(destQuery);

    if (originID == -1) cerr << "Origin not found: " << originQuery << endl;
    if (destID == -1) cerr << "Destination not found: " << destQuery << endl;

    DynArray<RouteEdge*> direct;
    DynArray<TwoHop> onehops(0);
    if (originID != -1 && destID != -1) {
        direct = find_direct_routes(graph, originID, destID);
        onehops = find_one_stop_routes(graph, originID, destID, 60);
    }

    // Main loop
    while (window.isOpen()) {
        simulationTime++;

        if (simulationTime % 30 == 0) {
            for (int i = 0; i < graph.portsArr.size(); ++i) {
                if (graph.portsArr[i]->dockSystem)
                    graph.portsArr[i]->dockSystem->update(simulationTime);
            }
            bookingSystem.updateBookingStatus(simulationTime);
        }

        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            // Mouse click handling for click-to-book
            if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mpos((float)ev.mouseButton.x, (float)ev.mouseButton.y);

                // Handle confirmation dialog clicks
                if (uiState.waitingForConfirmation) {
                    float panelW = 420.0f, panelH = 120.0f;
                    float px = (WINW - panelW) * 0.5f, py = (WINH - panelH) * 0.6f;
                    sf::FloatRect confirmRect(px + 60, py + 62, 120, 36);
                    sf::FloatRect cancelRect(px + 240, py + 62, 120, 36);

                    if (confirmRect.contains(mpos)) {
                        // Confirm booking
                        time_t t = time(nullptr);
                        tm tmnow;
#ifdef _WIN32
                        localtime_s(&tmnow, &t);
#else
                        localtime_r(&t, &tmnow);
#endif
                        char dateBuf[32];
                        sprintf_s(dateBuf, "%02d/%02d/%04d", tmnow.tm_mday, tmnow.tm_mon + 1, tmnow.tm_year + 1900);
                        string dateStr = string(dateBuf);

                        string shipName = "AutoShip_" + to_string(rand() % 10000);
                        string cargoType = "Containers";

                        DockQueueSystem** portDocks = new DockQueueSystem * [graph.portsArr.size()];
                        for (int i = 0; i < graph.portsArr.size(); ++i)
                            portDocks[i] = graph.portsArr[i]->dockSystem;

                        string bookingID = bookingSystem.bookRoute(
                            graph, uiState.originID, uiState.destID,
                            dateStr, shipName, cargoType, portDocks
                        );

                        delete[] portDocks;

                        if (!bookingID.empty()) {
                            uiState.bookingResultMsg = string("BOOKING CONFIRMED: ") + bookingID;
                        }
                        else {
                            uiState.bookingResultMsg = string("BOOKING FAILED: no route available");
                        }
                        uiState.showBookingResult = true;
                        uiState.waitingForConfirmation = false;
                        uiState.originID = -1;
                        uiState.destID = -1;
                        uiState.tentativePath = PathResult();
                    }
                    else if (cancelRect.contains(mpos)) {
                        uiState.waitingForConfirmation = false;
                        uiState.tentativePath = PathResult();
                        uiState.destID = -1;
                    }
                    continue;
                }

                if (uiState.showBookingResult) {
                    uiState.showBookingResult = false;
                    uiState.bookingResultMsg.clear();
                    continue;
                }

                // Normal port selection
                int clicked = getClickedPortID(graph, mpos, 12.0f);
                if (clicked == -1) continue;

                if (uiState.originID == -1) {
                    uiState.originID = clicked;
                    cout << "[UI] Origin selected: " << graph.get_port_by_id(clicked)->name << "\n";
                }
                else if (uiState.originID != -1 && uiState.destID == -1) {
                    if (clicked == uiState.originID) {
                        cout << "[UI] Destination cannot be same as origin.\n";
                        continue;
                    }
                    uiState.destID = clicked;
                    cout << "[UI] Destination selected: " << graph.get_port_by_id(clicked)->name << "\n";

                    if (useCostOptimization) {
                        uiState.tentativePath = pathFinder.find_shortest_path_dijkstra(
                            uiState.originID, uiState.destID, true);
                    }
                    else {
                        uiState.tentativePath = pathFinder.find_shortest_path_astar(
                            uiState.originID, uiState.destID, false);
                    }

                    if (!uiState.tentativePath.found) {
                        cout << "[UI] No feasible route found.\n";
                        uiState.bookingResultMsg = "No feasible route between selected ports.";
                        uiState.showBookingResult = true;
                        uiState.originID = -1;
                        uiState.destID = -1;
                        uiState.tentativePath = PathResult();
                        continue;
                    }

                    uiState.waitingForConfirmation = true;
                    cout << "[UI] Route found. Please confirm booking on-screen.\n";
                }
            }

            if (ev.type == sf::Event::KeyPressed) {
                // [Previous keyboard handlers remain the same - B, V, D, A, R, L, M, C, Q, F, X, E, P keys]
                // I'll include them for completeness but they're unchanged from original

                if (ev.key.code == sf::Keyboard::B) {
                    string shipName, cargoType, date;
                    cout << "\n=== SHIP ROUTE BOOKING ===" << endl;
                    cout << "Ship Name: ";
                    getline(cin, shipName);
                    if (shipName.empty()) shipName = "CargoShip_" + to_string(rand() % 1000);
                    cout << "Cargo Type: ";
                    getline(cin, cargoType);
                    if (cargoType.empty()) cargoType = "Containers";
                    cout << "Voyage Date (DD/MM/YYYY): ";
                    getline(cin, date);
                    if (date.empty()) date = "01/12/2024";

                    if (originID != -1 && destID != -1) {
                        DockQueueSystem** portDocks = new DockQueueSystem * [graph.portsArr.size()];
                        for (int i = 0; i < graph.portsArr.size(); ++i)
                            portDocks[i] = graph.portsArr[i]->dockSystem;

                        string bookingID = bookingSystem.bookRoute(graph, originID, destID,
                            date, shipName, cargoType, portDocks);

                        delete[] portDocks;
                        if (!bookingID.empty()) cout << "✓ Booking confirmed! ID: " << bookingID << endl;
                        else cout << "✗ Booking failed! No route available." << endl;
                    }
                }

                if (ev.key.code == sf::Keyboard::V) {
                    cout << "\n=== ALL BOOKINGS ===" << endl;
                    DynArray<ShipBooking>& bookings = bookingSystem.getAllBookings();
                    if (bookings.size() == 0) cout << "No bookings yet." << endl;
                    else {
                        for (int i = 0; i < bookings.size(); ++i) {
                            cout << "Booking " << (i + 1) << ": " << bookings[i].bookingID
                                << " - " << bookings[i].shipName
                                << " (" << bookings[i].cargoType << ")" << endl;
                            cout << "  Total Cost: $" << bookings[i].totalCost << endl;
                            cout << "  Total Time: " << (bookings[i].totalTime / 60) << " hours" << endl;
                        }
                    }
                }

                if (ev.key.code == sf::Keyboard::D) {
                    if (originID != -1 && destID != -1) {
                        currentPathResult = pathFinder.find_shortest_path_dijkstra(originID, destID, useCostOptimization);
                        algorithmRunning = true;
                        currentAlgorithmName = "Dijkstra";
                        animatingPath = true;
                        animationProgress = 0.0f;
                        animationClock.restart();
                        exploredPorts = pathFinder.get_explored_ports();
                        currentExploredCount = exploredPorts.size();
                        currentPathLength = currentPathResult.path.size();
                        print_path_result(graph, currentPathResult);
                    }
                }

                if (ev.key.code == sf::Keyboard::A) {
                    if (originID != -1 && destID != -1) {
                        currentPathResult = pathFinder.find_shortest_path_astar(originID, destID, useCostOptimization);
                        algorithmRunning = true;
                        currentAlgorithmName = "A*";
                        animatingPath = true;
                        animationProgress = 0.0f;
                        animationClock.restart();
                        exploredPorts = pathFinder.get_explored_ports();
                        currentExploredCount = exploredPorts.size();
                        currentPathLength = currentPathResult.path.size();
                        print_path_result(graph, currentPathResult);
                    }
                }

                if (ev.key.code == sf::Keyboard::R) {
                    algorithmRunning = false;
                    animatingPath = false;
                    animationProgress = 0.0f;
                    pathFinder.reset_visualization();
                    cout << "Visualization reset" << endl;
                }

                if (ev.key.code == sf::Keyboard::C) {
                    useCostOptimization = !useCostOptimization;
                    cout << "Toggled: " << (useCostOptimization ? "COST OPTIMIZATION" : "TIME OPTIMIZATION") << endl;
                }

                if (ev.key.code == sf::Keyboard::Q) {
                    if (graph.portsArr.size() > 0) {
                        int portIndex = rand() % graph.portsArr.size();
                        PortNode* p = graph.portsArr[portIndex];
                        string company = "MaerskLine";
                        if (p->companyStorages.size() > 0) {
                            company = p->companyStorages[rand() % p->companyStorages.size()]->companyName;
                        }
                        string shipName = "RandShip_" + to_string(rand() % 10000);
                        int processTime = 60 + (rand() % 241);
                        long arriveTime = simulationTime;

                        p->addShipToCompanyQueue(shipName, company, arriveTime, processTime);
                        if (p->dockSystem) p->dockSystem->enqueueShip(shipName, company, arriveTime, processTime);

                        cout << "[Q] Added ship '" << shipName << "' at " << p->name << endl;
                    }
                }

                if (ev.key.code == sf::Keyboard::F) {
                    cout << "Enter company name to filter: ";
                    string company;
                    getline(cin, company);
                    if (!company.empty()) {
                        filterByCompany = true;
                        filterCompany = company;
                        filterGraphByCompany(graph, company, filteredPorts);
                        cout << "Filtering by: " << company << " (" << filteredPorts.size() << " ports)" << endl;
                    }
                }

                if (ev.key.code == sf::Keyboard::X) {
                    filterByCompany = false;
                    filterCompany = "";
                    filteredPorts.length = 0;
                    cout << "Filter cleared" << endl;
                }

                if (ev.key.code == sf::Keyboard::E) {
                    showDockPanel = !showDockPanel;
                    cout << "Dock panel " << (showDockPanel ? "enabled" : "hidden") << endl;
                }

                if (ev.key.code == sf::Keyboard::P) {
                    cout << "\n=== USER PREFERENCES ===" << endl;
                    cout << "1. Add preferred company\n2. Add avoided port\n";
                    cout << "3. Set max voyage time\n4. Set max cost\nChoice: ";
                    int choice;
                    if (cin >> choice) {
                        cin.ignore();
                        switch (choice) {
                        case 1: {
                            cout << "Company name: ";
                            string company;
                            getline(cin, company);
                            if (!company.empty()) {
                                userPrefs.preferredCompanies.push(company);
                                cout << "Added " << company << endl;
                            }
                            break;
                        }
                        case 2: {
                            cout << "Port to avoid: ";
                            string port;
                            getline(cin, port);
                            if (!port.empty()) {
                                userPrefs.avoidedPorts.push(port);
                                cout << "Added " << port << endl;
                            }
                            break;
                        }
                        case 3: {
                            cout << "Max voyage time (hours): ";
                            cin >> userPrefs.maxVoyageTime;
                            cin.ignore();
                            cout << "Set to " << userPrefs.maxVoyageTime << " hours" << endl;
                            break;
                        }
                        case 4: {
                            cout << "Max cost ($): ";
                            cin >> userPrefs.maxCost;
                            cin.ignore();
                            cout << "Set to $" << userPrefs.maxCost << endl;
                            break;
                        }
                        }
                    }
                }
            }

            if (ev.type == sf::Event::MouseMoved) {
                sf::Vector2i mpos = sf::Mouse::getPosition(window);
                hoverPort = nullptr;
                hoverEdge = nullptr;
                hoverEdgeFrom = nullptr;
                for (int i = 0; i < graph.portsArr.size(); ++i) {
                    PortNode* p = graph.portsArr[i];
                    float dx = p->x - mpos.x;
                    float dy = p->y - mpos.y;
                    if (sqrt(dx * dx + dy * dy) <= 10.0f) { hoverPort = p; break; }
                }
                hoverEdge = find_edge_near(graph, (float)mpos.x, (float)mpos.y, &hoverEdgeFrom, 6.0f);
            }
        }

        // Update animation
        if (animatingPath && algorithmRunning) {
            float elapsed = animationClock.getElapsedTime().asSeconds();
            animationProgress = clampf(elapsed / 3.0f, 0.0f, 1.0f);
            if (animationProgress >= 1.0f) animatingPath = false;
        }

        // Drawing
        drawGradientBackground(window, WINW, WINH);

        if (haveMap) {
            sf::Sprite s(mapTex);
            float sx = (float)WINW / mapTex.getSize().x;
            float sy = (float)WINH / mapTex.getSize().y;
            s.setScale(sx, sy);
            s.setColor(sf::Color(255, 255, 255, 180));
            window.draw(s);
        }

        // Draw edges (with filtering support)
        if (filterByCompany) {
            for (int i = 0; i < graph.portsArr.size(); ++i) {
                PortNode* p = graph.portsArr[i];
                RouteEdge* e = p->edges;
                while (e) {
                    PortNode* dest = graph.get_port_by_id(e->destPortID);
                    if (dest) {
                        sf::Vertex line[2];
                        line[0].position = sf::Vector2f(p->x, p->y);
                        line[1].position = sf::Vector2f(dest->x, dest->y);
                        line[0].color = sf::Color(100, 100, 100, 50);
                        line[1].color = sf::Color(100, 100, 100, 50);
                        window.draw(line, 2, sf::Lines);
                    }
                    e = e->next;
                }
            }

            for (int i = 0; i < filteredPorts.size(); ++i) {
                PortNode* p = graph.get_port_by_id(filteredPorts[i]);
                if (!p) continue;
                RouteEdge* e = p->edges;
                while (e) {
                    if (e->company == filterCompany) {
                        PortNode* dest = graph.get_port_by_id(e->destPortID);
                        if (dest) drawEdge(window, p, dest, e, true);
                    }
                    e = e->next;
                }
            }
        }
        else {
            for (int i = 0; i < graph.portsArr.size(); ++i) {
                PortNode* p = graph.portsArr[i];
                RouteEdge* e = p->edges;
                while (e) {
                    PortNode* dest = graph.get_port_by_id(e->destPortID);
                    if (dest) drawEdge(window, p, dest, e, false);
                    e = e->next;
                }
            }
        }

        bookingSystem.drawBookedRoutes(window, graph, font);

        if (algorithmRunning) {
            for (int i = 0; i < exploredPorts.size(); ++i) {
                PortNode* p = graph.get_port_by_id(exploredPorts[i]);
                if (p) {
                    float prog = (exploredPorts.size() > 0) ? (float)i / exploredPorts.size() : 0.0f;
                    if (prog <= animationProgress) {
                        sf::CircleShape marker(9.0f);
                        marker.setOrigin(9.0f, 9.0f);
                        marker.setPosition(p->x, p->y);
                        int r = 100;
                        int gg = mini(255, 200 + int(55 * prog));
                        int b = maxi(100, 255 - int(155 * prog));
                        marker.setFillColor(sf::Color((sf::Uint8)r, (sf::Uint8)gg, (sf::Uint8)b, 200));
                        marker.setOutlineColor(sf::Color::White);
                        marker.setOutlineThickness(1.5f);
                        window.draw(marker);
                    }
                }
            }

            if (currentPathResult.found) {
                drawAnimatedPath(window, graph, currentPathResult.path, animationProgress, sf::Color::Green);
            }
        }

        // Draw ports
        for (int i = 0; i < graph.portsArr.size(); ++i) {
            PortNode* p = graph.portsArr[i];
            bool isOnPath = false;
            if (currentPathResult.found) {
                for (int j = 0; j < currentPathResult.path.size(); ++j) {
                    if (currentPathResult.path[j] == p->id) { isOnPath = true; break; }
                }
            }
            drawPort(window, p, font, (hoverPort == p), isOnPath);
            drawDockQueue(window, p, font);
        }

        // Draw UI panels
        if (hoverPort) {
            ostringstream ss;
            ss << "PORT: " << hoverPort->name << "\n"
                << "Daily Charge: $" << hoverPort->portCharge << "\n"
                << "Waiting Ships: " << hoverPort->getTotalWaitingShips() << "\n"
                << "Docked Ships: " << hoverPort->getTotalDockedShips() << "\n";
            drawInfoPanel(window, font, ss.str(), hoverPort->x + 20, hoverPort->y + 20, 280, 150);
        }

        // Controls panel
        string controlsText =
            "CONTROLS:\n"
            "B - Book Route | V - View Bookings\n"
            "D - Dijkstra | A - A* Algorithm\n"
            "C - Toggle Cost/Time | R - Reset\n"
            "Q - Add Ship | E - Dock Panel\n"
            "F - Filter Company | X - Clear Filter\n"
            "P - User Preferences\n\n"
            "Mode: " + string(useCostOptimization ? "COST" : "TIME");

        drawInfoPanel(window, font, controlsText, 10, WINH - 290, 250, 280);

        if (algorithmRunning) {
            drawAlgorithmStatus(window, font, algorithmRunning, currentAlgorithmName,
                currentExploredCount, currentPathLength, algorithmTime,
                animationProgress, WINW);
        }

        drawBookingInfo(window, bookingSystem, graph, font, WINW);

        if (showDockPanel) {
            ostringstream ss;
            ss << "DOCK STATUS:\n";
            for (int i = 0; i < mini(12, graph.portsArr.size()); ++i) {
                PortNode* p = graph.portsArr[i];
                ss << p->name << ": Q=" << p->getTotalWaitingShips()
                    << " D=" << p->getTotalDockedShips() << "\n";
            }
            drawInfoPanel(window, font, ss.str(), 10, 10, 360, 220);
        }

        if (hoverEdge) {
            string es = edgeSummary(graph, hoverEdge);
            sf::Vector2i mp = sf::Mouse::getPosition(window);
            drawInfoPanel(window, font, es, (float)mp.x + 12, (float)mp.y + 8, 320, 80);
        }

        window.display();
    }

    return 0;
}