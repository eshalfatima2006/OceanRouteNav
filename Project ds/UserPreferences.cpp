#include "UserPreferences.h"

using namespace std;

UserPreferences::UserPreferences() : maxVoyageTime(168), maxCost(1000000) {
    preferredCompanies = DynArray<string>(5);
    avoidedPorts = DynArray<string>(5);
}

bool edgeMatchesPreferences(RouteEdge* edge, const UserPreferences* prefs, Graph* g) {
    if (!edge) return false;
    if (!prefs || !g) return true; // if no prefs/graph provided, don't filter

    // Check company preference
    if (prefs->preferredCompanies.size() > 0) {
        bool companyOk = false;
        for (int i = 0; i < prefs->preferredCompanies.size(); ++i) {
            if (edge->company == prefs->preferredCompanies[i]) {
                companyOk = true;
                break;
            }
        }
        if (!companyOk) return false;
    }

    // Check avoided ports
    PortNode* dest = g->get_port_by_id(edge->destPortID);
    if (dest) {
        for (int i = 0; i < prefs->avoidedPorts.size(); ++i) {
            if (dest->name == prefs->avoidedPorts[i]) {
                return false;
            }
        }
    }

    // Check cost
    if (edge->cost > prefs->maxCost) return false;

    return true;
}

bool user_pref_edge_filter(RouteEdge* edge, void* ctx) {
    if (!ctx) return true;
    EdgeFilterCtx* c = static_cast<EdgeFilterCtx*>(ctx);
    if (!c || !c->prefs || !c->graph) return true;
    return edgeMatchesPreferences(edge, c->prefs, c->graph);
}

void filterGraphByCompany(Graph& graph, const string& company, DynArray<int>& result) {
    result.length = 0;

    for (int i = 0; i < graph.portsArr.size(); ++i) {
        PortNode* port = graph.portsArr[i];
        bool hasCompanyRoute = false;

        // Check outgoing edges
        RouteEdge* edge = port->edges;
        while (edge) {
            if (edge->company == company) {
                hasCompanyRoute = true;
                break;
            }
            edge = edge->next;
        }

        // Check incoming edges
        for (int j = 0; j < graph.portsArr.size(); ++j) {
            PortNode* other = graph.portsArr[j];
            RouteEdge* e = other->edges;
            while (e) {
                if (e->destPortID == port->id && e->company == company) {
                    hasCompanyRoute = true;
                    break;
                }
                e = e->next;
            }
            if (hasCompanyRoute) break;
        }

        if (hasCompanyRoute) {
            result.push(port->id);
        }
    }
}