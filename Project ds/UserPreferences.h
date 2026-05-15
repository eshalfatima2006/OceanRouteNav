#pragma once
#ifndef USER_PREFERENCES_H
#define USER_PREFERENCES_H

#include "CommonStructures.h"
#include <string>

// User preferences for route filtering
struct UserPreferences {
    DynArray<std::string> preferredCompanies;
    DynArray<std::string> avoidedPorts;
    int maxVoyageTime; // in hours
    int maxCost;

    UserPreferences();
};

// Edge filter context for PathFinder callback
struct EdgeFilterCtx {
    UserPreferences* prefs;
    Graph* graph;
};

bool edgeMatchesPreferences(RouteEdge* edge, const UserPreferences* prefs, Graph* g);

bool user_pref_edge_filter(RouteEdge* edge, void* ctx);

// Subgraph filtering by company
void filterGraphByCompany(Graph& graph, const std::string& company,
    DynArray<int>& result);

#endif // USER_PREFERENCES_H