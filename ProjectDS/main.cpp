#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <cstring>
//#include <unordered_map>
#include "CommonStructures.h"
#include "PriorityQueue.h" 
#include "BookingSystem.h"
#include "PathFinder.h"
#include "DockQueue.h"

using namespace std;

// Utility: convert string to lowercase (ASCII only)
static string toLowerStr(const string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = std::tolower(static_cast<unsigned char>(out[i]));
    }
    return out;
}

// -------------------------------
// Time parsing
// -------------------------------
static long parseDateTimeToMinutes(const string& dateStr, const string& timeStr) {
    int D = 0, M = 0, Y = 0;
    int hh = 0, mm = 0;

    if (sscanf_s(dateStr.c_str(), "%d/%d/%d", &D, &M, &Y) != 3) return 0;
    if (sscanf_s(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) { hh = 0; mm = 0; }

    long totalMinutes = (((Y * 12 + M) * 31 + D) * 24 + hh) * 60 + mm;
    return totalMinutes;
}

static int parseTimeToMinutes(const string& timeStr) {
    int hh = 0, mm = 0;
    if (sscanf_s(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) {
        hh = 0; mm = 0;
    }
    return hh * 60 + mm;
}

static void fillEdgeTimestamps(RouteEdge* e) {
    // Parse date just to validate, but we don't use it for absolute timing
    // Instead, treat all routes as available on any day
    e->departTS = parseTimeToMinutes(e->departStr);
    e->arriveTS = parseTimeToMinutes(e->arriveStr);

    // If arrival is earlier than departure (next day), add 24 hours
    if (e->arriveTS < e->departTS) {
        e->arriveTS += 24 * 60;
    }
}

// -------------------------------
// Utility: deterministic placement of ports
// -------------------------------
// Geographic coordinate mapping for 1200x800 window - NO STL VERSION
struct PortCoordEntry {
    const char* name;
    float x, y;
};

static PortCoordEntry* createPortCoordinates(int& count) {
    // Fix: Increase NUM_PORTS to 40 to match the highest index used (coords[39])
    const int NUM_PORTS = 40;
    PortCoordEntry* coords = new PortCoordEntry[NUM_PORTS];

    // North America
    coords[0] = { "NewYork", 265, 240 };
    coords[1] = { "Montreal", 280, 220 };
    coords[2] = { "LosAngeles", 130, 260 };
    coords[3] = { "Vancouver", 110, 200 };

    // Europe
    coords[4] = { "London", 485, 215 };
    coords[5] = { "Dublin", 470, 210 };
    coords[6] = { "Oslo", 515, 180 };
    coords[7] = { "Stockholm", 530, 175 };
    coords[8] = { "Helsinki", 550, 170 };
    coords[9] = { "Copenhagen", 520, 195 };
    coords[10] = { "Hamburg", 515, 205 };
    coords[11] = { "Rotterdam", 500, 210 };
    coords[12] = { "Antwerp", 498, 212 };
    coords[13] = { "Lisbon", 465, 250 };
    coords[14] = { "Marseille", 502, 235 };
    coords[15] = { "Genoa", 515, 235 };
    coords[16] = { "Athens", 555, 250 };
    coords[17] = { "Istanbul", 565, 242 };

    // Middle East
    coords[18] = { "Alexandria", 565, 270 };
    coords[19] = { "Dubai", 640, 285 };
    coords[20] = { "AbuDhabi", 642, 287 };
    coords[21] = { "Doha", 635, 285 };
    coords[22] = { "Jeddah", 600, 295 };

    // South Asia
    coords[23] = { "Karachi", 500, 285 };
    coords[24] = { "Mumbai", 680, 295 };
    coords[25] = { "Colombo", 695, 310 };
    coords[26] = { "Chittagong", 730, 300 };

    // East Asia
    coords[27] = { "Singapore", 760, 320 };
    coords[28] = { "Jakarta", 770, 330 };
    coords[29] = { "Manila", 815, 305 };
    coords[30] = { "HongKong", 790, 300 };
    coords[31] = { "Shanghai", 810, 270 };
    coords[32] = { "Busan", 830, 255 };
    coords[33] = { "Osaka", 850, 255 };
    coords[34] = { "Tokyo", 860, 250 };

    // Africa
    coords[35] = { "CapeTown", 540, 440 };
    coords[36] = { "Durban", 565, 420 };
    coords[37] = { "PortLouis", 640, 405 };

    // Oceania
    coords[38] = { "Melbourne", 900, 425 };
    coords[39] = { "Sydney", 920, 415 };

    count = NUM_PORTS; // Update count
    return coords;
}
// Updated function to assign real geographic positions
// Updated function to assign real geographic positions - NO STL VERSION
static void assign_port_positions(Graph& g, int width, int height) {
    int coordCount = 0;
    PortCoordEntry* coordMap = createPortCoordinates(coordCount);

    for (size_t i = 0; i < g.portsArr.size(); ++i) {
        PortNode* p = g.portsArr[i];
        bool found = false;

        // Try to find exact match first
        for (int j = 0; j < coordCount; j++) {
            if (strcmp(p->name.c_str(), coordMap[j].name) == 0) {
                p->x = coordMap[j].x;
                p->y = coordMap[j].y;
                found = true;
                break;
            }
        }

        // If not found, try case-insensitive search
        if (!found) {
            string lowerName = toLowerStr(p->name);
            for (int j = 0; j < coordCount; j++) {
                string lowerMapName = toLowerStr(coordMap[j].name);
                if (lowerName == lowerMapName) {
                    p->x = coordMap[j].x;
                    p->y = coordMap[j].y;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            // Fallback: place unknown ports in bottom-left corner
            p->x = 50;
            p->y = height - 50;
            cerr << "Warning: No coordinates found for port: " << p->name << "\n";
        }

        // Ensure coordinates are within bounds
        if (p->x < 40) p->x = 40;
        if (p->y < 40) p->y = 40;
        if (p->x > width - 40) p->x = width - 40;
        if (p->y > height - 40) p->y = height - 40;
    }

    delete[] coordMap; // Clean up
}
// -------------------------------
// Parsing functions
// -------------------------------
static bool load_port_charges(Graph& g, const string& filepath) {
    ifstream in(filepath);
    if (!in.is_open()) {
        cerr << "Failed to open PortCharges file: " << filepath << "\n";
        return false;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string pname;
        int charge;
        if (!(ss >> pname >> charge)) continue;
        int id = g.get_port_id(pname);
        if (id == -1) id = g.add_port(pname);
        PortNode* p = g.get_port_by_id(id);
        if (p) p->portCharge = charge;
    }
    in.close();
    return true;
}

static bool load_routes(Graph& g, const string& filepath) {
    ifstream in(filepath);
    if (!in.is_open()) {
        cerr << "Failed to open Routes file: " << filepath << "\n";
        return false;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        // Format:
        // Origin Destination DD/MM/YYYY HH:MM HH:MM Cost Company
        // But company may contain underscores. We'll parse first 6 tokens, rest is company.
        istringstream ss(line);
        string origin, dest, date, depart, arrive, costStr;
        if (!(ss >> origin >> dest >> date >> depart >> arrive >> costStr)) continue;
        string company;
        getline(ss, company);
        // company may start with space
        if (!company.empty() && company[0] == ' ') company.erase(0, 1);
        int cost = atoi(costStr.c_str());
        int oid = g.get_port_id(origin);
        if (oid == -1) oid = g.add_port(origin);
        int did = g.get_port_id(dest);
        if (did == -1) did = g.add_port(dest);
        RouteEdge* e = new RouteEdge();
        e->destPortID = did;
        e->dateStr = date;
        e->departStr = depart;
        e->arriveStr = arrive;
        e->cost = cost;
        e->company = company;
        fillEdgeTimestamps(e);
        g.add_edge(oid, e);
    }
    in.close();
    return true;
}

// -------------------------------
// Route lookup: direct routes
// -------------------------------
static DynArray<RouteEdge*> find_direct_routes(Graph& g, int originID, int destID) {
    DynArray<RouteEdge*> result(8);
    PortNode* p = g.get_port_by_id(originID);
    if (!p) return result;
    RouteEdge* e = p->edges;
    while (e) {
        if (e->destPortID == destID) result.push(e);
        e = e->next;
    }
    return result;
}

// -------------------------------
// Find 1-stop connecting routes with basic time feasibility:
// origin -> mid , mid -> destination
// require: arrival_at_mid + transfer_minutes <= depart_of_next
// -------------------------------
struct TwoHop {
    RouteEdge* first;
    RouteEdge* second;
};

static DynArray<TwoHop> find_one_stop_routes(Graph& g, int originID, int destID, int minTransferMinutes = 60) {
    DynArray<TwoHop> out(16);
    PortNode* p = g.get_port_by_id(originID);
    if (!p) return out;
    // iterate outgoing from origin
    RouteEdge* e1 = p->edges;
    while (e1) {
        int mid = e1->destPortID;
        if (mid == destID) { e1 = e1->next; continue; } // skip direct
        PortNode* midNode = g.get_port_by_id(mid);
        if (!midNode) { e1 = e1->next; continue; }
        // look for edges mid -> dest
        RouteEdge* e2 = midNode->edges;
        while (e2) {
            if (e2->destPortID == destID) {
                // feasibility: arrival at mid + minTransfer <= depart of e2
                long arrival = e1->arriveTS;
                long departNext = e2->departTS;
                // If departNext < arrival -> perhaps next day; but our timestamps already use same date epoch.
                if (departNext < arrival) {
                    // assume next day for second leg
                    departNext += 24 * 60;
                }
                if (arrival + minTransferMinutes <= departNext) {
                    TwoHop th; th.first = e1; th.second = e2;
                    out.push(th);
                }
            }
            e2 = e2->next;
        }
        e1 = e1->next;
    }
    return out;
}

// -------------------------------
// Display helper: prepare textual description for edges
// -------------------------------
static string edgeSummary(Graph& g, RouteEdge* e) {
    if (!e) return "";
    ostringstream ss;
    PortNode* dest = g.get_port_by_id(e->destPortID);
    ss << "-> " << (dest ? dest->name : "?")
        << " | " << e->dateStr << " " << e->departStr << "-" << e->arriveStr
        << " | $" << e->cost << " | " << e->company;
    return ss.str();
}

// -------------------------------
// SFML drawing helpers
// -------------------------------
static sf::Color routeColorByCost(int cost) {
    // low cost = greenish, high cost = reddish
    int c = min(255, cost / 200); // scale down
    int r = min(255, c * 2);
    int g = max(0, 200 - c);
    return sf::Color(r, g, 120, 200);
}


static void drawPort(sf::RenderWindow& win, PortNode* p, sf::Font& font, bool highlight = false) {
    if (!p) return;
    sf::CircleShape circ(6);
    circ.setOrigin(6, 6);
    circ.setPosition(p->x, p->y);

    if (highlight) {
        circ.setRadius(9);
        circ.setOrigin(9, 9);
        circ.setFillColor(sf::Color::Yellow);
        circ.setOutlineColor(sf::Color::Red);
        circ.setOutlineThickness(2);
    }
    else {
        circ.setFillColor(sf::Color(70, 130, 255));
        circ.setOutlineColor(sf::Color::White);
        circ.setOutlineThickness(1);
    }

    win.draw(circ);

    // draw name
    sf::Text t;
    t.setFont(font);
    t.setCharacterSize(12);
    t.setString(p->name);
    t.setPosition(p->x + 8, p->y - 8);
    t.setFillColor(sf::Color::Black);
    win.draw(t);
}

static void drawEdge(sf::RenderWindow& win, PortNode* a, PortNode* b, RouteEdge* e) {
    if (!a || !b || !e) return;
    sf::Vertex line[2];
    line[0].position = sf::Vector2f(a->x, a->y);
    line[1].position = sf::Vector2f(b->x, b->y);
    sf::Color col = routeColorByCost(e->cost);
    line[0].color = col;
    line[1].color = col;
    win.draw(line, 2, sf::Lines);
}

// New function to draw path with highlighting
static void drawPath(sf::RenderWindow& win, Graph& g, DynArray<int>& path, sf::Color color = sf::Color::Green, float thickness = 3.0f) {
    if (path.size() < 2) return;

    for (size_t i = 0; i < path.size() - 1; ++i) {
        PortNode* a = g.get_port_by_id(path[i]);
        PortNode* b = g.get_port_by_id(path[i + 1]);
        if (a && b) {
            // Draw a thicker line for the path
            sf::Vertex line[2];
            line[0].position = sf::Vector2f(a->x, a->y);
            line[1].position = sf::Vector2f(b->x, b->y);
            line[0].color = color;
            line[1].color = color;
            win.draw(line, 2, sf::Lines);

            // Highlight the ports on the path
            sf::CircleShape marker(8);
            marker.setOrigin(8, 8);
            marker.setPosition(a->x, a->y);
            marker.setFillColor(sf::Color(255, 255, 0, 150)); // Yellow highlight
            win.draw(marker);
        }
    }

    // Highlight the last port
    if (path.size() > 0) {
        PortNode* last = g.get_port_by_id(path[path.size() - 1]);
        if (last) {
            sf::CircleShape marker(8);
            marker.setOrigin(8, 8);
            marker.setPosition(last->x, last->y);
            marker.setFillColor(sf::Color(255, 255, 0, 150));
            win.draw(marker);
        }
    }
}

// -------------------------------
// Dock queue drawing function
// -------------------------------
// Replace drawDockQueue function:
static void drawDockQueue(sf::RenderWindow& win, PortNode* port, sf::Font& font) {
    if (!port || !port->dockSystem) return;

    int totalQueueSize = port->dockSystem->get_total_queue_size();
    int totalDocked = port->dockSystem->get_total_docked_count();

    // Draw queue indicator (colored circle)
    sf::CircleShape indicator(5);
    indicator.setOrigin(5, 5);
    indicator.setPosition(port->x - 12, port->y - 12);

    // Color based on queue status
    if (totalQueueSize == 0) {
        indicator.setFillColor(sf::Color::Green);
    }
    else if (totalQueueSize < 3) {
        indicator.setFillColor(sf::Color::Yellow);
    }
    else {
        indicator.setFillColor(sf::Color::Red);
    }

    indicator.setOutlineColor(sf::Color::White);
    indicator.setOutlineThickness(1);
    win.draw(indicator);

    // Draw queue info if queue exists
    if (totalQueueSize > 0) {
        sf::Text queueText;
        queueText.setFont(font);
        queueText.setCharacterSize(9);
        queueText.setString("Q:" + to_string(totalQueueSize));
        queueText.setPosition(port->x - 18, port->y - 22);
        queueText.setFillColor(sf::Color::White);
        queueText.setStyle(sf::Text::Bold);
        win.draw(queueText);
    }

    // Draw dock occupancy
    sf::Text dockText;
    dockText.setFont(font);
    dockText.setCharacterSize(9);
    dockText.setString("D:" + to_string(totalDocked));
    dockText.setPosition(port->x + 10, port->y - 22);
    dockText.setFillColor(sf::Color::Cyan);
    win.draw(dockText);
}
// Add this function to main.cpp
//void test_connectivity(Graph& graph, const string& origin, const string& dest) {
//    cout << "\n=== CONNECTIVITY TEST ===" << endl;
//
//    int startID = graph.get_port_id(origin);
//    int endID = graph.get_port_id(dest);
//
//    if (startID == -1) {
//        cout << origin << " not found in graph!" << endl;
//        return;
//    }
//    if (endID == -1) {
//        cout << dest << " not found in graph!" << endl;
//        return;
//    }
//
//    cout << origin << " -> ID: " << startID << endl;
//    cout << dest << " -> ID: " << endID << endl;
//
//    // Check direct connections
//    PortNode* start = graph.get_port_by_id(startID);
//    int directCount = 0;
//    RouteEdge* edge = start->edges;
//
//    cout << "\nDirect routes from " << origin << ":" << endl;
//    while (edge) {
//        directCount++;
//        PortNode* destPort = graph.get_port_by_id(edge->destPortID);
//        cout << "  -> " << (destPort ? destPort->name : "?")
//            << " (ID: " << edge->destPortID << ")"
//            << " Cost: $" << edge->cost << endl;
//        edge = edge->next;
//    }
//    cout << "Total direct routes: " << directCount << endl;
//
//    // Simple BFS to check connectivity
//    bool* visited = new bool[graph.portsArr.size()]();
//    DynArray<int> queue(50);
//    queue.push(startID);
//    visited[startID] = true;
//
//    int hops = 0;
//    bool found = false;
//
//    while (queue.size() > 0 && hops < 10) {
//        int currentSize = queue.size();
//
//        for (int i = 0; i < currentSize; i++) {
//            int currentID = queue[i];
//
//            if (currentID == endID) {
//                found = true;
//                cout << "\n*** FOUND CONNECTION in " << hops << " hops! ***" << endl;
//                break;
//            }
//
//            PortNode* current = graph.get_port_by_id(currentID);
//            if (!current) continue;
//
//            edge = current->edges;
//            while (edge) {
//                if (!visited[edge->destPortID]) {
//                    visited[edge->destPortID] = true;
//                    queue.push(edge->destPortID);
//                }
//                edge = edge->next;
//            }
//        }
//
//        if (found) break;
//
//        // Remove processed nodes
//        for (int i = 0; i < currentSize; i++) {
//            for (int j = 0; j < queue.size() - 1; j++) {
//                queue[j] = queue[j + 1];
//            }
//            queue.length--;
//        }
//
//        hops++;
//    }
//
//    if (!found) {
//        cout << "\n*** NO CONNECTION found within 10 hops ***" << endl;
//    }
//
//    delete[] visited;
//}

// -------------------------------
// Main program
// -------------------------------
int main() {
    Graph graph;

    // Load files
    bool ok1 = load_port_charges(graph, "PortCharges.txt");
    bool ok2 = load_routes(graph, "Routes.txt");
    if (!ok2) {
        cerr << "Routes.txt not loaded. Exiting.\n";
        return 1;
    }

    // Create booking system
    BookingSystem bookingSystem;

    // Create path finder
    PathFinder pathFinder(&graph);

    // ========== DOCKING SYSTEM INITIALIZATION ==========
    cout << "\n=== INITIALIZING DOCKING SYSTEM ===" << endl;
    for (size_t i = 0; i < graph.portsArr.size(); ++i) {
        PortNode* port = graph.portsArr[i];

        // Assign 2-4 docks based on port importance
        int docks = 2;
        if (port->name == "Singapore" || port->name == "Rotterdam" ||
            port->name == "Shanghai" || port->name == "Dubai") {
            docks = 4;  // Major hubs get more docks
        }
        else if (port->name == "HongKong" || port->name == "Busan" ||
            port->name == "LosAngeles" || port->name == "NewYork") {
            docks = 3;  // Major ports get 3 docks
        }

        port->dockSystem = new DockQueueSystem();
        cout << "Port " << port->name << ": " << docks << " docking slots" << endl;
    }
    // ===================================================

    // assign positions for rendering
    const int WINW = 1200, WINH = 800;
    assign_port_positions(graph, WINW, WINH);

    // create SFML window
    sf::RenderWindow window(sf::VideoMode(WINW, WINH), "OceanRoute Nav - Complete System");
    window.setFramerateLimit(60);

    // load font
    sf::Font font;
    if (!font.loadFromFile("Arial.ttf")) {
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            cerr << "Failed to load fonts. Labels may not render.\n";
        }
    }

    // try background map
    sf::Texture mapTex;
    bool haveMap = false;
    if (mapTex.loadFromFile("world_map.png")) {
        haveMap = true;
    }

    // UI state
    string originQuery = "";
    string destQuery = "";
    PortNode* hoverPort = nullptr;



    // Pathfinding state
    bool algorithmRunning = false;
    bool useCostOptimization = true;
    PathResult currentPathResult;
    DynArray<int> exploredPorts;
    sf::Clock algorithmClock;  // ADD THIS LINE
    float algorithmTime = 0.0f; // ADD THIS LINE
    long simulationTime = 0;

    // After loading routes in main():
   // test_connectivity(graph, "Mumbai", "NewYork");

    cout << "=== OceanRoute Nav - Complete System ===" << endl;
    cout << "Enter origin port name: ";
    getline(cin, originQuery);
    cout << "Enter destination port name: ";
    getline(cin, destQuery);

    int originID = graph.get_port_id(originQuery);
    int destID = graph.get_port_id(destQuery);

    if (originID == -1) {
        cerr << "Origin not found: " << originQuery << endl;
    }
    else {
        cout << "Origin ID: " << originID << " (" << graph.get_port_by_id(originID)->name << ")" << endl;
    }
    if (destID == -1) {
        cerr << "Destination not found: " << destQuery << endl;
    }
    else {
        cout << "Destination ID: " << destID << " (" << graph.get_port_by_id(destID)->name << ")" << endl;
    }

    // Prepare direct + one-stop results
    DynArray<RouteEdge*> direct;
    DynArray<TwoHop> onehops(0);
    if (originID != -1 && destID != -1) {
        direct = find_direct_routes(graph, originID, destID);
        onehops = find_one_stop_routes(graph, originID, destID, 60);
    }

    // Console print summary
    cout << "\nDirect routes found: " << direct.size() << endl;
    for (size_t i = 0; i < direct.size(); ++i) {
        cout << edgeSummary(graph, direct[i]) << endl;
    }
    cout << "\nOne-stop connections found: " << onehops.size() << endl;
    for (size_t i = 0; i < onehops.size(); ++i) {
        cout << edgeSummary(graph, onehops[i].first) << "  =>  " << edgeSummary(graph, onehops[i].second) << endl;
    }

    // Instructions
    cout << "\n=== Controls ===" << endl;
    cout << "B - Book a Ship Route (Feature #2)" << endl;
    cout << "D - Run Dijkstra's Algorithm (Shortest Path)" << endl;
    cout << "A - Run A* Algorithm" << endl;
    cout << "C - Toggle between Cost/Time optimization" << endl;
    cout << "R - Reset path visualization" << endl;
    cout << "Q - Add random ship to random port" << endl;
    cout << "E - Show dock queue status" << endl;
    cout << "V - View all bookings" << endl;
    cout << "Current optimization: " << (useCostOptimization ? "COST" : "TIME") << endl;
    cout << "\n=== Port Indicators ===" << endl;
    cout << "Green circle: Empty queue" << endl;
    cout << "Yellow circle: 1-2 ships waiting" << endl;
    cout << "Red circle: 3+ ships waiting" << endl;
    cout << "Purple routes: Booked routes" << endl;
    cout << "Q:X - Queue size" << endl;
    cout << "D:X/Y - Docked ships / Total docks" << endl;

    // Main loop
    while (window.isOpen()) {
        // Update simulation time
        simulationTime++;

        // Update dock queues periodically
        if (simulationTime % 30 == 0) {  // Update every 30 frames
            for (size_t i = 0; i < graph.portsArr.size(); ++i) {
                if (graph.portsArr[i]->dockSystem) {
                    graph.portsArr[i]->dockSystem->update(simulationTime);
                }
            }
            // Update booking system
            bookingSystem.updateBookingStatus(simulationTime);
        }
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (ev.type == sf::Event::KeyPressed) {
                // ========== BOOKING SYSTEM CONTROLS ==========
                if (ev.key.code == sf::Keyboard::B) {
                    // Book a route
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
                        // Create portDocks array for booking system
                        DockQueueSystem** portDocks = new DockQueueSystem * [graph.portsArr.size()];
                        for (size_t i = 0; i < graph.portsArr.size(); ++i) {
                            portDocks[i] = graph.portsArr[i]->dockSystem;
                        }

                        string bookingID = bookingSystem.bookRoute(graph, originID, destID,
                            date, shipName, cargoType, portDocks);

                        delete[] portDocks;

                        if (!bookingID.empty()) {
                            cout << "✅ Booking confirmed! ID: " << bookingID << endl;
                        }
                        else {
                            cout << "❌ Booking failed! No route available." << endl;
                        }
                    }
                }

                if (ev.key.code == sf::Keyboard::V) {
                    // View all bookings
                    cout << "\n=== ALL BOOKINGS ===" << endl;
                    DynArray<ShipBooking>& bookings = bookingSystem.getAllBookings();
                    if (bookings.size() == 0) {
                        cout << "No bookings yet." << endl;
                    }
                    else {
                        for (size_t i = 0; i < bookings.size(); ++i) {
                            cout << "Booking " << (i + 1) << ": " << bookings[i].bookingID
                                << " - " << bookings[i].shipName
                                << " (" << bookings[i].cargoType << ")" << endl;
                            cout << "  Route: ";
                            for (size_t j = 0; j < bookings[i].legs.size(); ++j) {
                                PortNode* from = graph.get_port_by_id(bookings[i].legs[j].fromPortID);
                                PortNode* to = graph.get_port_by_id(bookings[i].legs[j].toPortID);
                                if (from && to) {
                                    cout << from->name;
                                    if (j < bookings[i].legs.size() - 1) cout << " -> ";
                                }
                            }
                            if (bookings[i].legs.size() > 0) {
                                PortNode* last = graph.get_port_by_id(bookings[i].legs[bookings[i].legs.size() - 1].toPortID);
                                if (last) cout << " -> " << last->name;
                            }
                            cout << endl;
                            cout << "  Total Cost: $" << bookings[i].totalCost << endl;
                            cout << "  Total Time: " << (bookings[i].totalTime / 60) << " hours" << endl;
                        }
                    }
                }

                // ========== DOCKING CONTROLS ==========
                if (ev.key.code == sf::Keyboard::Q) {
                    // Add random ship to random port
                    if (graph.portsArr.size() > 0) {
                        int portIndex = rand() % graph.portsArr.size();
                        PortNode* port = graph.portsArr[portIndex];

                        if (port->dockSystem) {
                            // Generate random ship
                            string shipNames[] = { "Atlantic", "Pacific", "Indian", "Mediterranean",
                                                  "Caribbean", "Baltic", "Arabian", "SouthChina" };
                            string companies[] = { "Maersk", "MSC", "COSCO", "Evergreen", "CMA_CGM",
                                                  "HapagLloyd", "YangMing", "ONE", "ZIM", "PIL" };

                            string shipName = shipNames[rand() % 8] + "_" + to_string(rand() % 1000);
                            string company = companies[rand() % 10];
                            int processTime = 60 + rand() % 180;  // 1-4 hours in simulation time

                            port->dockSystem->enqueueShip(shipName, company, simulationTime, processTime);
                        }
                    }
                }

                if (ev.key.code == sf::Keyboard::E) {
                    // Print dock status for all ports
                    cout << "\n=== DOCK QUEUE STATUS ===" << endl;
                    for (size_t i = 0; i < graph.portsArr.size(); ++i) {
                        PortNode* port = graph.portsArr[i];
                        if (port->dockSystem && port->dockSystem->get_queue_size() > 0) {
                            cout << port->name << ": ";
                            port->dockSystem->print_status();
                        }
                    }
                }
                if (ev.key.code == sf::Keyboard::D) {
                    // Run Dijkstra
                    if (originID != -1 && destID != -1) {
                        cout << "Running Dijkstra's Algorithm..." << endl;

                        // TIMER CODE ADDED HERE
                        algorithmClock.restart();
                        currentPathResult = pathFinder.find_shortest_path_dijkstra(originID, destID, useCostOptimization);
                        algorithmTime = algorithmClock.getElapsedTime().asSeconds();

                        algorithmRunning = true;
                        exploredPorts = pathFinder.get_explored_ports();

                        // TIMING OUTPUT ADDED HERE
                        cout << "Algorithm completed in " << algorithmTime << " seconds" << endl;

                        if (currentPathResult.found) {
                            cout << " Path found! Total cost: $" << currentPathResult.totalCost << endl;
                            cout << "Path: ";
                            for (size_t i = 0; i < currentPathResult.path.size(); ++i) {
                                PortNode* p = graph.get_port_by_id(currentPathResult.path[i]);
                                if (p) cout << p->name;
                                if (i < currentPathResult.path.size() - 1) cout << " -> ";
                            }
                            cout << endl;
                        }
                        else {
                            cout << " No path found between " << originQuery << " and " << destQuery << endl;
                        }
                    }
                }

                if (ev.key.code == sf::Keyboard::A) {
                    // Run A*
                    if (originID != -1 && destID != -1) {
                        cout << "Running A* Algorithm..." << endl;

                        algorithmClock.restart();
                        currentPathResult = pathFinder.find_shortest_path_astar(originID, destID, useCostOptimization);
                        algorithmTime = algorithmClock.getElapsedTime().asSeconds();

                        algorithmRunning = true;
                        exploredPorts = pathFinder.get_explored_ports();

                        cout << "A* Algorithm completed in " << algorithmTime << " seconds" << endl;

                        if (currentPathResult.found) {
                            cout << " A* Path found! Total cost: $" << currentPathResult.totalCost << endl;
                            cout << "A* Path: ";
                            for (size_t i = 0; i < currentPathResult.path.size(); ++i) {
                                PortNode* p = graph.get_port_by_id(currentPathResult.path[i]);
                                if (p) cout << p->name;
                                if (i < currentPathResult.path.size() - 1) cout << " -> ";
                            }
                            cout << endl;
                        }
                        else {
                            cout << " A* No path found between " << originQuery << " and " << destQuery << endl;
                        }
                    }
                }

                if (ev.key.code == sf::Keyboard::C) {
                    // Toggle cost/time optimization
                    useCostOptimization = !useCostOptimization;
                    cout << "Optimization changed to: " << (useCostOptimization ? "COST" : "TIME") << endl;
                }

                if (ev.key.code == sf::Keyboard::R) {
                    // Reset visualization
                    algorithmRunning = false;
                    pathFinder.reset_visualization();
                    cout << "Visualization reset" << endl;
                }
            }

            if (ev.type == sf::Event::MouseMoved) {
                sf::Vector2i mpos = sf::Mouse::getPosition(window);
                hoverPort = nullptr;
                // find nearest port within radius
                for (size_t i = 0; i < graph.portsArr.size(); ++i) {
                    PortNode* p = graph.portsArr[i];
                    float dx = p->x - mpos.x;
                    float dy = p->y - mpos.y;
                    if (sqrt(dx * dx + dy * dy) <= 10.0f) { hoverPort = p; break; }
                }
            }
        }

        window.clear(sf::Color(30, 30, 40));
        if (haveMap) {
            sf::Sprite s(mapTex);
            float sx = (float)WINW / mapTex.getSize().x;
            float sy = (float)WINH / mapTex.getSize().y;
            s.setScale(sx, sy);
            window.draw(s);
        }

        // draw all edges (thin)
        for (size_t i = 0; i < graph.portsArr.size(); ++i) {
            PortNode* p = graph.portsArr[i];
            RouteEdge* e = p->edges;
            while (e) {
                PortNode* dest = graph.get_port_by_id(e->destPortID);
                if (dest) drawEdge(window, p, dest, e);
                e = e->next;
            }
        }

        // Draw booked routes
        bookingSystem.drawBookedRoutes(window, graph, font);

        // Visualize algorithm exploration
        if (algorithmRunning) {
            // Draw explored ports in light blue
            for (size_t i = 0; i < exploredPorts.size(); ++i) {
                PortNode* p = graph.get_port_by_id(exploredPorts[i]);
                if (p) {
                    sf::CircleShape marker(7);
                    marker.setOrigin(7, 7);
                    marker.setPosition(p->x, p->y);
                    marker.setFillColor(sf::Color(100, 200, 255, 180));
                    window.draw(marker);
                }
            }

            // Draw the final path
            if (currentPathResult.found) {
                drawPath(window, graph, currentPathResult.path, sf::Color::Green, 4.0f);
            }
        }

        // highlight direct + one-stop routes (keep your existing code)
        if (originID != -1 && destID != -1 && !algorithmRunning) {
            // direct - draw thicker lines
            for (size_t i = 0; i < direct.size(); ++i) {
                RouteEdge* e = direct[i];
                PortNode* a = graph.get_port_by_id(originID);
                PortNode* b = graph.get_port_by_id(e->destPortID);
                if (a && b) {
                    sf::Vertex quad[2];
                    quad[0].position = { a->x, a->y };
                    quad[1].position = { b->x, b->y };
                    quad[0].color = sf::Color::Cyan;
                    quad[1].color = sf::Color::Cyan;
                    window.draw(quad, 2, sf::Lines);
                }
            }
            // one-stop
            for (size_t i = 0; i < onehops.size(); ++i) {
                TwoHop th = onehops[i];
                PortNode* a = graph.get_port_by_id(originID);
                PortNode* mid = graph.get_port_by_id(th.first->destPortID);
                PortNode* b = graph.get_port_by_id(destID);
                if (a && mid) {
                    sf::Vertex seg1[2]; seg1[0].position = { a->x,a->y }; seg1[1].position = { mid->x,mid->y };
                    seg1[0].color = sf::Color(255, 200, 0); seg1[1].color = sf::Color(255, 200, 0);
                    window.draw(seg1, 2, sf::Lines);
                }
                if (mid && b) {
                    sf::Vertex seg2[2]; seg2[0].position = { mid->x,mid->y }; seg2[1].position = { b->x,b->y };
                    seg2[0].color = sf::Color(255, 200, 0); seg2[1].color = sf::Color(255, 200, 0);
                    window.draw(seg2, 2, sf::Lines);
                }
            }
        }

        // draw ports (highlight hover) and dock queues
        for (size_t i = 0; i < graph.portsArr.size(); ++i) {
            PortNode* p = graph.portsArr[i];
            drawPort(window, p, font, (hoverPort == p));
            drawDockQueue(window, p, font);  // ← ADD THIS LINE
        }

        // tooltip if hovering a port
        if (hoverPort) {
            sf::RectangleShape box;
            box.setFillColor(sf::Color(255, 255, 255, 240));
            box.setOutlineColor(sf::Color::Black);
            box.setOutlineThickness(1);
            box.setPosition(hoverPort->x + 12, hoverPort->y + 12);
            box.setSize({ 260, 120 });
            window.draw(box);
            sf::Text t;
            t.setFont(font);
            t.setCharacterSize(13);
            ostringstream ss;
            ss << "Port: " << hoverPort->name << "\nCharge/day: $" << hoverPort->portCharge << "\nOutgoing routes:\n";
            RouteEdge* e = hoverPort->edges;
            int cnt = 0;
            while (e && cnt < 6) {
                ss << "  " << edgeSummary(graph, e) << "\n";
                e = e->next; cnt++;
            }
            if (e) ss << "  ...\n";
            t.setString(ss.str());
            t.setFillColor(sf::Color::Black);
            t.setPosition(hoverPort->x + 16, hoverPort->y + 16);
            window.draw(t);
        }

        // Draw instructions on screen
        sf::Text instructions;
        instructions.setFont(font);
        instructions.setCharacterSize(14);
        instructions.setFillColor(sf::Color::White);
        instructions.setPosition(10, 10);
        instructions.setString(
            "Controls:\n"
            "B - Book Ship Route\n"
            "V - View Bookings\n"
            "D - Dijkstra's Algorithm\n"
            "A - A* Algorithm\n"
            "C - Toggle Cost/Time\n"
            "R - Reset path\n"
            "Q - Add ship to port\n"
            "E - Show dock status\n"
            "Mode: " + string(useCostOptimization ? "COST" : "TIME")
        );

        sf::RectangleShape instructionBg;
        instructionBg.setFillColor(sf::Color(0, 0, 0, 180));
        instructionBg.setSize(sf::Vector2f(200, 140));
        instructionBg.setPosition(5, 5);
        window.draw(instructionBg);
        window.draw(instructions);

        window.display();
    }

    return 0;
}
