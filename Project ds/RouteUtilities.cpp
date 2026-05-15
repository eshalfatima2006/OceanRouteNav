#include "RouteUtilities.h"
#include <cstdio>
#include <sstream>
#include <iostream>
#include <climits>
#include "PathFinder.h"

using namespace std;

long parseDateTimeToMinutes(const string& dateStr, const string& timeStr) {
    int D = 0, M = 0, Y = 0;
    int hh = 0, mm = 0;

#ifdef _MSC_VER
    
    if (sscanf_s(dateStr.c_str(), "%d/%d/%d", &D, &M, &Y) != 3) return 0;
    if (sscanf_s(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) { hh = 0; mm = 0; }
#else
  
    if (sscanf(dateStr.c_str(), "%d/%d/%d", &D, &M, &Y) != 3) return 0;
    if (sscanf(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) { hh = 0; mm = 0; }
#endif

    long totalMinutes = (((Y * 12 + M) * 31 + D) * 24 + hh) * 60 + mm;
    return totalMinutes;
}

int parseTimeToMinutes(const string& timeStr) {
    int hh = 0, mm = 0;
#ifdef _MSC_VER
    if (sscanf_s(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) {
        hh = 0; mm = 0;
    }
#else
    if (sscanf(timeStr.c_str(), "%d:%d", &hh, &mm) != 2) {
        hh = 0; mm = 0;
    }
#endif
    return hh * 60 + mm;
}

void fillEdgeTimestamps(RouteEdge* e) {
    e->departTS = parseTimeToMinutes(e->departStr);
    e->arriveTS = parseTimeToMinutes(e->arriveStr);

    if (e->arriveTS < e->departTS) {
        e->arriveTS += 24 * 60;
    }
}

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

DynArray<TwoHop> find_one_stop_routes(Graph& g, int originID, int destID, int minTransferMinutes) {
    DynArray<TwoHop> out(16);
    PortNode* p = g.get_port_by_id(originID);
    if (!p) return out;
    RouteEdge* e1 = p->edges;
    while (e1) {
        int mid = e1->destPortID;
        if (mid == destID) { e1 = e1->next; continue; }
        PortNode* midNode = g.get_port_by_id(mid);
        if (!midNode) { e1 = e1->next; continue; }
        RouteEdge* e2 = midNode->edges;
        while (e2) {
            if (e2->destPortID == destID) {
                long arrival = e1->arriveTS;
                long departNext = e2->departTS;
                if (departNext < arrival) departNext += 24 * 60;
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

string edgeSummary(Graph& g, RouteEdge* e) {
    if (!e) return "";
    ostringstream ss;
    PortNode* dest = g.get_port_by_id(e->destPortID);
    ss << "-> " << (dest ? dest->name : "?")
        << " | " << e->dateStr << " " << e->departStr << "-" << e->arriveStr
        << " | $" << e->cost << " | " << e->company;
    return ss.str();
}

RouteEdge* find_best_edge_between(PortNode* from, int toID) {
    if (!from) return nullptr;
    RouteEdge* e = from->edges;
    RouteEdge* best = nullptr;
    int bestCost = INT_MAX;
    while (e) {
        if (e->destPortID == toID) {
            if (e->cost < bestCost) {
                bestCost = e->cost;
                best = e;
            }
        }
        e = e->next;
    }
    return best;
}

void print_path_result(Graph& g, const PathResult& res) {
    if (!res.found) {
        cout << "No path found.\n";
        return;
    }
    cout << "Path found:\n";
    for (int i = 0; i < res.path.size(); ++i) {
        PortNode* p = g.get_port_by_id(res.path[i]);
        if (p) {
            cout << p->name;
        }
        else {
            cout << "Port(" << res.path[i] << ")";
        }
        if (i + 1 < res.path.size()) {
            PortNode* from = g.get_port_by_id(res.path[i]);
            int toID = res.path[i + 1];
            RouteEdge* edge = find_best_edge_between(from, toID);
            if (edge) {
                cout << "  ->  " << g.get_port_by_id(toID)->name
                    << " | " << edge->company
                    << " | " << edge->departStr << "-" << edge->arriveStr
                    << " | $" << edge->cost << "\n";
            }
            else {
                cout << "  ->  " << (g.get_port_by_id(toID) ? g.get_port_by_id(toID)->name : "Unknown")
                    << "  (edge info unavailable)\n";
            }
        }
        else {
            cout << "\n";
        }
    }
    cout << "Total cost (approx): $" << res.totalCost << "\n";
}