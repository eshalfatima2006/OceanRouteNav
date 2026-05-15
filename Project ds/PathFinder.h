#pragma once
#include "CommonStructures.h"
#include "PriorityQueue.h"

struct PathResult {
    DynArray<int> path; // sequence of port IDs
    int totalCost;
    long totalTime;
    bool found;
    PathResult() : path(DynArray<int>(0)), totalCost(0), totalTime(0), found(false) {}
};

class PathFinder {
public:
    // Edge filter callback: return true to allow edge, false to skip.
    typedef bool (*EdgeFilterFn)(RouteEdge* edge, void* ctx);

private:
    Graph* graph;
    DynArray<int> exploredPorts;
    DynArray<int> currentPath;
    bool verbose; // when true -> print debug; default false

    // optional edge filter
    EdgeFilterFn edgeFilter;
    void* edgeFilterCtx;

    bool shouldStop();

public:
    PathFinder(Graph* g, bool verboseLogging = false);

    // allow toggling runtime verbosity
    void set_verbose(bool v) { verbose = v; }
    bool is_verbose() const { return verbose; }

    // set an edge filter callback (optional)
    void set_edge_filter(EdgeFilterFn fn, void* ctx) { edgeFilter = fn; edgeFilterCtx = ctx; }

    // Dijkstra's algorithm
    PathResult find_shortest_path_dijkstra(int startID, int endID, bool useCost = true);

    // A* algorithm
    PathResult find_shortest_path_astar(int startID, int endID, bool useCost = true);

    // For visualization
    void reset_visualization();
    DynArray<int> get_explored_ports() const { return exploredPorts; }
    DynArray<int> get_current_path() const { return currentPath; }

private:
    // Helper functions
    int heuristic_cost(int currentID, int goalID, bool useCost);
    DynArray<int> reconstruct_path(int* prev, int startID, int endID, int numPorts);
    int calculate_time_cost(RouteEdge* edge);
};
