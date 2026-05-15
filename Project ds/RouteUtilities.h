#ifndef ROUTE_UTILITIES_H
#define ROUTE_UTILITIES_H

#include "CommonStructures.h"
#include <string>

struct PathResult;

// Time parsing utilities
long parseDateTimeToMinutes(const std::string& dateStr, const std::string& timeStr);
int parseTimeToMinutes(const std::string& timeStr);
void fillEdgeTimestamps(RouteEdge* e);

// Route finding structures
struct TwoHop {
    RouteEdge* first;
    RouteEdge* second;
};

// Route search functions
DynArray<RouteEdge*> find_direct_routes(Graph& g, int originID, int destID);

DynArray<TwoHop> find_one_stop_routes(Graph& g, int originID, int destID,
    int minTransferMinutes = 60);

RouteEdge* find_best_edge_between(PortNode* from, int toID);

// Route display functions
std::string edgeSummary(Graph& g, RouteEdge* e);
void print_path_result(Graph& g, const PathResult& res);

#endif // ROUTE_UTILITIES_H