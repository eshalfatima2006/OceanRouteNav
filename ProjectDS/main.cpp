// main.cpp
// SFML 2 + Custom graph for Routes.txt + PortCharges.txt
// Build (example):

// Place Routes.txt, PortCharges.txt, optional world_map.png in the same folder.

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

// -------------------------------
// Simple dynamic array for pointers (modern C++)
// -------------------------------
template <typename T>
struct DynArray {
    T* data;
    size_t capacity;
    size_t length;
    DynArray(size_t cap = 16) {
        capacity = cap;
        length = 0;
        data = new T[capacity];
        if (!data) { std::cerr << "allocation failed\n"; std::exit(1); }
    }
    ~DynArray() { delete[] data; }
    void push(T value) {
        if (length >= capacity) {
            size_t nc = capacity * 2;
            T* nd = new T[nc];
            if (!nd) { std::cerr << "allocation failed\n"; std::exit(1); }
            for (size_t i = 0; i < length; ++i) nd[i] = data[i];
            delete[] data;
            data = nd; capacity = nc;
        }
        data[length++] = value;
    }
    T& operator[](size_t i) { return data[i]; }
    size_t size() const { return length; }
};

// -------------------------------
// RouteEdge (adjacency node)
// -------------------------------
struct RouteEdge {
    int destPortID;
    std::string dateStr;
    std::string departStr;
    std::string arriveStr;
    long departTS; // minutes since epoch
    long arriveTS;
    int cost;
    std::string company;
    RouteEdge* next;
    RouteEdge() : destPortID(-1), departTS(0), arriveTS(0), cost(0), next(nullptr) {}
};

// -------------------------------
// PortNode
// -------------------------------
struct PortNode {
    int id;
    std::string name;
    int portCharge;
    float x, y; // screen coords for rendering
    RouteEdge* edges; // head of adjacency list
    PortNode() : id(-1), portCharge(0), x(0), y(0), edges(nullptr) {}
};

// -------------------------------
// Simple chained hash map (string -> int) (modern C++)
// -------------------------------
struct HashEntry {
    std::string key;
    int val;
    std::unique_ptr<HashEntry> next;
    HashEntry(const std::string& k, int v) : key(k), val(v), next(nullptr) {}
};

struct SimpleHash {
    std::unique_ptr<HashEntry>* buckets;
    size_t nb;
    SimpleHash(size_t n = 1031) { // prime-ish
        nb = n;
        buckets = new std::unique_ptr<HashEntry>[nb];
    }
    ~SimpleHash() {
        delete[] buckets;
    }
    unsigned long hashString(const std::string& s) {
        unsigned long h = 5381;
        for (char c : s) h = ((h << 5) + h) + (unsigned char)std::tolower(c);
        return h % nb;
    }
    void insert(const std::string& key, int val) {
        unsigned long idx = hashString(key);
        HashEntry* e = buckets[idx].get();
        while (e) {
            if (strcasecmp(e->key.c_str(), key.c_str()) == 0) { e->val = val; return; }
            e = e->next.get();
        }
        auto ne = std::make_unique<HashEntry>(key, val);
        ne->next = std::move(buckets[idx]);
        buckets[idx] = std::move(ne);
    }
    int get(const std::string& key) {
        unsigned long idx = hashString(key);
        HashEntry* e = buckets[idx].get();
        while (e) {
            if (strcasecmp(e->key.c_str(), key.c_str()) == 0) return e->val;
            e = e->next.get();
        }
        return -1;
    }
private:
    static int strcasecmp(const char* a, const char* b) {
        while (*a && *b) {
            char ca = std::tolower(*a);
            char cb = std::tolower(*b);
            if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
            ++a; ++b;
        }
        return (unsigned char)std::tolower(*a) - (unsigned char)std::tolower(*b);
    }
};

// -------------------------------
// Graph
// -------------------------------
struct Graph {
    DynArray<PortNode*> portsArr; // array of PortNode*
    SimpleHash nameToID;
    size_t portCount;
    size_t routeCount;
    Graph() : portsArr(32), nameToID(1031), portCount(0), routeCount(0) {}
    ~Graph() {
        for (size_t i = 0; i < portsArr.size(); ++i) {
            PortNode* p = portsArr[i];
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
        p->id = (int)portCount;
        p->name = name;
        p->portCharge = 0;
        p->edges = nullptr;
        portsArr.push(p);
        nameToID.insert(name, (int)portCount);
        ++portCount;
        return p->id;
    }
    PortNode* get_port_by_id(int id) {
        if (id < 0 || (size_t)id >= portsArr.size()) return nullptr;
        return portsArr[id];
    }
    int get_port_id(const std::string& name) {
        return nameToID.get(name);
    }
    void add_edge(int originID, RouteEdge* edge) {
        if (originID < 0 || originID >= (int)portsArr.size()) return;
        PortNode* p = portsArr[originID];
        edge->next = p->edges;
        p->edges = edge;
        ++routeCount;
    }
};

// -------------------------------
// Time parsing
// -------------------------------
long parseDateTimeToMinutes(const std::string& dateStr, const std::string& timeStr) {
    int D = 0, M = 0, Y = 0;
    int hh = 0, mm = 0;

    if (sscanf_s(dateStr.c_str(), "%d/%d/%d", &D, &M, &Y) != 3) return 0;
    if (sscanf_s(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) { hh = 0; mm = 0; }

    long totalMinutes = (((Y * 12 + M) * 31 + D) * 24 + hh) * 60 + mm;
    return totalMinutes;
}

void fillEdgeTimestamps(RouteEdge* e) {
    e->departTS = parseDateTimeToMinutes(e->dateStr, e->departStr);
    long arrivalCandidate = parseDateTimeToMinutes(e->dateStr, e->arriveStr);
    if (arrivalCandidate < e->departTS) arrivalCandidate += 24 * 60;
    e->arriveTS = arrivalCandidate;
}

// -------------------------------
// Utility: deterministic placement of ports
// -------------------------------
void assign_port_positions(Graph& g, int width, int height) {
    for (size_t i = 0; i < g.portsArr.size(); ++i) {
        PortNode* p = g.portsArr[i];
        unsigned long h = 5381;
        for (char c : p->name) h = ((h << 5) + h) + (unsigned char)std::tolower(c);
        float angle = (h % 360) * 3.14159265f / 180.0f;
        float radius = 0.25f * std::min(width, height) + (h % (int)(0.4f * std::min(width, height)));
        float cx = width / 2.0f + radius * std::cos(angle);
        float cy = height / 2.0f + radius * std::sin(angle);
        if (cx < 40) cx = 40; if (cy < 40) cy = 40;
        if (cx > width - 40) cx = width - 40;
        if (cy > height - 40) cy = height - 40;
        p->x = cx; p->y = cy;
    }
}



// -------------------------------
// Parsing functions
// -------------------------------

bool load_port_charges(Graph& g, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        std::cerr << "Failed to open PortCharges file: " << filepath << "\n";
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string pname;
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

bool load_routes(Graph& g, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        std::cerr << "Failed to open Routes file: " << filepath << "\n";
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // Format:
        // Origin Destination DD/MM/YYYY HH:MM HH:MM Cost Company
        // But company may contain underscores. We'll parse first 6 tokens, rest is company.
        std::istringstream ss(line);
        std::string origin, dest, date, depart, arrive, costStr;
        if (!(ss >> origin >> dest >> date >> depart >> arrive >> costStr)) continue;
        std::string company;
        std::getline(ss, company);
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
DynArray<RouteEdge*> find_direct_routes(Graph& g, int originID, int destID) {
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
// (if depart time earlier than arrival - we use timestamps so it should be larger)
// -------------------------------
struct TwoHop {
    RouteEdge* first;
    RouteEdge* second;
};

DynArray<TwoHop> find_one_stop_routes(Graph& g, int originID, int destID, int minTransferMinutes = 60) {
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
std::string edgeSummary(Graph& g, RouteEdge* e) {
    if (!e) return "";
    std::ostringstream ss;
    PortNode* dest = g.get_port_by_id(e->destPortID);
    ss << "-> " << (dest ? dest->name : "?")
        << " | " << e->dateStr << " " << e->departStr << "-" << e->arriveStr
        << " | $" << e->cost << " | " << e->company;
    return ss.str();
}

// -------------------------------
// SFML drawing helpers
// -------------------------------
sf::Color routeColorByCost(int cost) {
    // low cost = greenish, high cost = reddish
    int c = std::min(255, cost / 200); // scale down
    int r = std::min(255, c * 2);
    int g = std::max(0, 200 - c);
    return sf::Color(r, g, 120, 200);
}

void drawPort(sf::RenderWindow& win, PortNode* p, sf::Font& font, bool highlight = false) {
    if (!p) return;
    sf::CircleShape circ(6);
    circ.setOrigin(6, 6);
    circ.setPosition(p->x, p->y);
    if (highlight) {
        circ.setRadius(9); circ.setOrigin(9, 9);
        circ.setFillColor(sf::Color::Yellow);
    }
    else {
        circ.setFillColor(sf::Color::White);
    }
    win.draw(circ);
    // draw name
    sf::Text t;
    t.setFont(font);
    t.setCharacterSize(12);
    t.setString(p->name);
    t.setPosition(p->x + 8, p->y - 8);
    t.setFillColor(sf::Color::White);
    win.draw(t);
}

void drawEdge(sf::RenderWindow& win, PortNode* a, PortNode* b, RouteEdge* e) {
    if (!a || !b || !e) return;
    sf::Vertex line[2];
    line[0].position = sf::Vector2f(a->x, a->y);
    line[1].position = sf::Vector2f(b->x, b->y);
    sf::Color col = routeColorByCost(e->cost);
    line[0].color = col;
    line[1].color = col;
    win.draw(line, 2, sf::Lines);
}

// -------------------------------
// Main program
// -------------------------------
int main() {
    Graph graph;

    // Load files
    bool ok1 = load_port_charges(graph, "PortCharges.txt"); // not fatal
    bool ok2 = load_routes(graph, "Routes.txt");
    if (!ok2) {
        std::cerr << "Routes.txt not loaded. Exiting.\n";
        return 1;
    }

    // assign positions for rendering
    const int WINW = 1200, WINH = 800;
    assign_port_positions(graph, WINW, WINH);

    // create SFML window
    sf::RenderWindow window(sf::VideoMode(WINW, WINH), "OceanRoute Nav - Graph Viewer");
    window.setFramerateLimit(60);

    // load font
    sf::Font font;
    if (!font.loadFromFile("Arial.ttf")) {
        // try system fallback
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Failed to load fonts. Labels may not render.\n";
        }
    }

    // try background map
    sf::Texture mapTex;
    bool haveMap = false;
    if (mapTex.loadFromFile("world_map.png")) {
        haveMap = true;
    }

    // UI state
    std::string originQuery = "";
    std::string destQuery = "";
    bool typingOrigin = false;
    bool typingDest = false;
    PortNode* hoverPort = nullptr;
    RouteEdge* hoverEdge = nullptr;

    // We'll do a simple UI: type origin and destination in console before window? But user wanted SFML display.
    // Provide minimal in-window guidance text and allow console input:
    std::cout << "Enter origin port name (case-insensitive, exact token): ";
    std::getline(std::cin, originQuery);
    std::cout << "Enter destination port name: ";
    std::getline(std::cin, destQuery);

    int originID = graph.get_port_id(originQuery);
    int destID = graph.get_port_id(destQuery);
    if (originID == -1) {
        std::cerr << "Origin not found: " << originQuery << "\n";
    }
    else {
        std::cout << "Origin ID: " << originID << "\n";
    }
    if (destID == -1) {
        std::cerr << "Destination not found: " << destQuery << "\n";
    }
    else {
        std::cout << "Destination ID: " << destID << "\n";
    }

    // Prepare direct + one-stop results
    DynArray<RouteEdge*> direct;
    DynArray<TwoHop> onehops(0);
    if (originID != -1 && destID != -1) {
        direct = find_direct_routes(graph, originID, destID);
        onehops = find_one_stop_routes(graph, originID, destID, 60);
    }

    // Console print summary
    std::cout << "\nDirect routes found: " << direct.size() << "\n";
    for (size_t i = 0; i < direct.size(); ++i) {
        std::cout << edgeSummary(graph, direct[i]) << "\n";
    }
    std::cout << "\nOne-stop connections found: " << onehops.size() << "\n";
    for (size_t i = 0; i < onehops.size(); ++i) {
        std::cout << edgeSummary(graph, onehops[i].first) << "  =>  " << edgeSummary(graph, onehops[i].second) << "\n";
    }

    // Main loop
    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
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
            // scale to fit
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

        // highlight direct + one-stop routes
        if (originID != -1 && destID != -1) {
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

        // draw ports (highlight hover)
        for (size_t i = 0; i < graph.portsArr.size(); ++i) {
            PortNode* p = graph.portsArr[i];
            drawPort(window, p, font, (hoverPort == p));
        }

        // tooltip if hovering a port: show outgoing routes count
        if (hoverPort) {
            sf::RectangleShape box;
            box.setFillColor(sf::Color(0, 0, 0, 180));
            box.setOutlineColor(sf::Color::White);
            box.setOutlineThickness(1);
            box.setPosition(hoverPort->x + 12, hoverPort->y + 12);
            box.setSize({ 260, 120 });
            window.draw(box);
            sf::Text t;
            t.setFont(font);
            t.setCharacterSize(13);
            std::ostringstream ss;
            ss << "Port: " << hoverPort->name << "\nCharge/day: $" << hoverPort->portCharge << "\nOutgoing routes:\n";
            RouteEdge* e = hoverPort->edges;
            int cnt = 0;
            while (e && cnt < 6) {
                ss << "  " << edgeSummary(graph, e) << "\n";
                e = e->next; cnt++;
            }
            if (e) ss << "  ...\n";
            t.setString(ss.str());
            t.setFillColor(sf::Color::White);
            t.setPosition(hoverPort->x + 16, hoverPort->y + 16);
            window.draw(t);
        }

        window.display();
    }

    return 0;
}
