#include "PathFinder.h"
#include <iostream>
#include <climits>
#include <cmath>
using namespace std;

PathFinder::PathFinder(Graph* g, bool verboseLogging) : graph(g), verbose(verboseLogging),
    edgeFilter(nullptr), edgeFilterCtx(nullptr)
{
    exploredPorts = DynArray<int>(50);
    currentPath = DynArray<int>(20);
}
PathResult PathFinder::find_shortest_path_dijkstra(int startID, int endID, bool useCost) {

    reset_visualization();

    int numPorts = (int)graph->portsArr.size();
    if (startID < 0 || startID >= numPorts || endID < 0 || endID >= numPorts) {
        if (verbose) cout << "ERROR: Invalid port IDs! startID=" << startID << ", endID=" << endID << ", numPorts=" << numPorts << endl;
        PathResult result;
        result.found = false;
        result.path = DynArray<int>(0);
        return result;
    }

    // Initialize arrays
    int* dist = new int[numPorts];
    int* prev = new int[numPorts];
    bool* visited = new bool[numPorts];

    for (int i = 0; i < numPorts; i++) {
        dist[i] = INT_MAX;
        prev[i] = -1;
        visited[i] = false;
    }
    dist[startID] = 0;

    PriorityQueue pq(numPorts);
    pq.push(startID, 0);

    int iterations = 0;
    int maxIterations = numPorts * 10; // Safety limit

    while (!pq.is_empty() && iterations < maxIterations) {
        if (shouldStop()) break;

        iterations++;
        PQNode current = pq.pop();
        int currentID = current.portID;

        // Safety check - invalid node
        if (currentID == -1) break;

        // Debug output (disabled by default)
        if (verbose && iterations % 100 == 0) {
            cout << "Iteration " << iterations << ": Exploring port " << currentID;
            cout << " (Queue size: " << pq.get_size() << ")" << endl;
        }

        // Add to explored for visualization
        exploredPorts.push(currentID);

        if (currentID == endID) {
            if (verbose) cout << " Path found after " << iterations << " iterations!" << endl;
            // Path found!
            DynArray<int> path = reconstruct_path(prev, startID, endID, numPorts);

            PathResult result;
            result.path = path;
            result.totalCost = dist[endID];
            result.totalTime = 0;
            result.found = true;

            delete[] dist;
            delete[] prev;
            delete[] visited;

            return result;
        }

        if (visited[currentID]) continue;
        visited[currentID] = true;

        // Explore neighbors using your Graph structure
        PortNode* currentPort = graph->get_port_by_id(currentID);
        if (!currentPort) continue;

        RouteEdge* edge = currentPort->edges;
        int neighborCount = 0;

        while (edge != nullptr) {
            int neighborID = edge->destPortID;
            neighborCount++;

            // Apply optional edge filter (user preferences / subgraph)
            if (edgeFilter && !edgeFilter(edge, edgeFilterCtx)) {
                edge = edge->next;
                continue;
            }

            // Debug output (disabled by default)
            if (verbose) {
                PortNode* neighborPort = graph->get_port_by_id(neighborID);
                cout << "  Exploring edge: " << currentID << " -> " << neighborID;
                if (neighborPort) cout << " (" << neighborPort->name << ")";
                cout << " cost: $" << edge->cost << endl;
            }

            // Self-loop protection
            if (neighborID == currentID) {
                if (verbose) cout << "WARNING: Self-loop detected at port " << currentID << endl;
                edge = edge->next;
                continue;
            }

            if (!visited[neighborID] && neighborID >= 0 && neighborID < numPorts) {
                int edgeCost = useCost ? edge->cost : calculate_time_cost(edge);

                // Cost validation (log only if verbose)
                if ((edgeCost < 0 || edgeCost > 100000) && verbose) {
                    cout << "WARNING: Invalid edge cost " << edgeCost << " from " << currentID << " to " << neighborID << endl;
                }
                if (edgeCost < 0) {
                    edge = edge->next;
                    continue;
                }

                int newDist = dist[currentID] + edgeCost;

                if (newDist < dist[neighborID]) {
                    dist[neighborID] = newDist;
                    prev[neighborID] = currentID;
                    pq.push(neighborID, newDist);
                }
            }
            edge = edge->next;
        }

        if (!verbose && neighborCount == 0) {
            // suppressed minor info
        } else if (verbose && neighborCount == 0) {
            cout << "Port " << currentID << " has no outgoing edges!" << endl;
        }
    }

    if (iterations >= maxIterations) {
        if (verbose) cout << " Dijkstra stopped after " << maxIterations << " iterations (safety limit)" << endl;
    }
    else if (pq.is_empty()) {
        if (verbose) cout << "No path found after " << iterations << " iterations" << endl;
    }

    // No path found
    delete[] dist;
    delete[] prev;
    delete[] visited;

    PathResult result;
    result.found = false;
    result.path = DynArray<int>(0);
    result.totalCost = 0;
    result.totalTime = 0;
    return result;
}

PathResult PathFinder::find_shortest_path_astar(int startID, int endID, bool useCost) {
    reset_visualization();
    int numPorts = (int)graph->portsArr.size();
    if (startID < 0 || startID >= numPorts || endID < 0 || endID >= numPorts) {
        if (verbose) cout << "ERROR: Invalid port IDs! startID=" << startID << ", endID=" << endID << ", numPorts=" << numPorts << endl;
        PathResult result;
        result.found = false;
        result.path = DynArray<int>(0);
        return result;
    }


    // Initialize arrays
    int* gScore = new int[numPorts];
    int* fScore = new int[numPorts];
    int* prev = new int[numPorts];
    bool* visited = new bool[numPorts];

    for (int i = 0; i < numPorts; i++) {
        gScore[i] = INT_MAX;
        fScore[i] = INT_MAX;
        prev[i] = -1;
        visited[i] = false;
    }

    gScore[startID] = 0;
    fScore[startID] = heuristic_cost(startID, endID, useCost);

    PriorityQueue pq(numPorts);
    pq.push(startID, fScore[startID]);

    int iterations = 0;
    int maxIterations = numPorts * 10; // Safety limit

    while (!pq.is_empty() && iterations < maxIterations) {
        if (shouldStop()) break;

        iterations++;
        PQNode current = pq.pop();
        int currentID = current.portID;

        // Safety check - invalid node
        if (currentID == -1) break;

        // Debug output every 100 iterations (disabled by default)
        if (verbose && iterations % 100 == 0) {
            cout << "A* Iteration " << iterations << ": Exploring port " << currentID;
            cout << " (Queue size: " << pq.get_size() << ")" << endl;
        }

        // Add to explored for visualization
        exploredPorts.push(currentID);

        if (currentID == endID) {
            if (verbose) cout << " A* Path found after " << iterations << " iterations!" << endl;
            DynArray<int> path = reconstruct_path(prev, startID, endID, numPorts);

            PathResult result;
            result.path = path;
            result.totalCost = gScore[endID];
            result.totalTime = 0;
            result.found = true;

            delete[] gScore;
            delete[] fScore;
            delete[] prev;
            delete[] visited;

            return result;
        }

        if (visited[currentID]) continue;
        visited[currentID] = true;

        // Explore neighbors
        PortNode* currentPort = graph->get_port_by_id(currentID);
        if (!currentPort) continue;

        RouteEdge* edge = currentPort->edges;
        int neighborCount = 0;

        while (edge != nullptr) {
            int neighborID = edge->destPortID;
            neighborCount++;

            // Apply optional edge filter
            if (edgeFilter && !edgeFilter(edge, edgeFilterCtx)) {
                edge = edge->next;
                continue;
            }

            if (verbose) {
                PortNode* neighborPort = graph->get_port_by_id(neighborID);
                cout << "  Exploring edge: " << currentID << " -> " << neighborID;
                if (neighborPort) cout << " (" << neighborPort->name << ")";
                cout << " cost: $" << edge->cost << endl;
            }

            // Self-loop protection
            if (neighborID == currentID) {
                if (verbose) cout << "WARNING: Self-loop detected at port " << currentID << endl;
                edge = edge->next;
                continue;
            }

            if (!visited[neighborID] && neighborID >= 0 && neighborID < numPorts) {
                int edgeCost = useCost ? edge->cost : calculate_time_cost(edge);

                if ((edgeCost < 0 || edgeCost > 100000) && verbose) {
                    cout << "WARNING: Invalid edge cost " << edgeCost << " from " << currentID << " to " << neighborID << endl;
                }

                if (edgeCost < 0) {
                    edge = edge->next;
                    continue;
                }

                int tentativeG = (gScore[currentID] == INT_MAX) ? INT_MAX : gScore[currentID] + edgeCost;

                if (tentativeG < gScore[neighborID]) {
                    prev[neighborID] = currentID;
                    gScore[neighborID] = tentativeG;
                    fScore[neighborID] = (tentativeG == INT_MAX) ? INT_MAX : (tentativeG + heuristic_cost(neighborID, endID, useCost));
                    pq.push(neighborID, fScore[neighborID]);
                }
            }
            edge = edge->next;
        }

        if (neighborCount == 0 && verbose) {
            cout << "Port " << currentID << " has no outgoing edges!" << endl;
        }
    }

    if (iterations >= maxIterations) {
        if (verbose) cout << " A* stopped after " << maxIterations << " iterations (safety limit)" << endl;
    }
    else if (pq.is_empty()) {
        if (verbose) cout << " A* No path found after " << iterations << " iterations" << endl;
    }

    delete[] gScore;
    delete[] fScore;
    delete[] prev;
    delete[] visited;

    PathResult result;
    result.found = false;
    result.path = DynArray<int>(0);
    result.totalCost = 0;
    result.totalTime = 0;
    return result;
}

int PathFinder::heuristic_cost(int currentID, int goalID, bool useCost) {
    // Use Euclidean distance on assigned port coordinates as a heuristic.
    // Scale heuristics to approximate cost/time units so A* is informed.
    PortNode* a = graph->get_port_by_id(currentID);
    PortNode* b = graph->get_port_by_id(goalID);
    if (!a || !b) return 0;

    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dist = sqrtf(dx * dx + dy * dy); // pixels

    // Convert pixel distance to a heuristic unit:
    // - For cost optimization, assume cost roughly proportional to distance (scale factor)
    // - For time optimization, assume travel time roughly proportional (smaller scale)
    const float COST_SCALE = 8.0f;   // tuneable
    const float TIME_SCALE = 1.5f;   // tuneable (minutes per pixel)
    int h = (int)(dist * (useCost ? COST_SCALE : TIME_SCALE));
    return h;
}

DynArray<int> PathFinder::reconstruct_path(int* prev, int startID, int endID, int numPorts) {
    DynArray<int> path(20);
    int current = endID;

    // Backtrack from end to start
    while (current != -1) {
        path.push(current);
        current = prev[current];
        if (path.size() > (size_t)numPorts) break; // Safety check
    }

    // Reverse the path to get start->end order
    for (int i = 0; i < path.size() / 2; i++) {
        int temp = path[i];
        path[i] = path[path.size() - 1 - i];
        path[path.size() - 1 - i] = temp;
    }

    // Store for visualization
    currentPath.length = 0;
    for (int i = 0; i < path.size(); i++) {
        currentPath.push(path[i]);
    }

    return path;
}

int PathFinder::calculate_time_cost(RouteEdge* edge) {
    if (!edge) return 0;

    // With new time system, arrival might be less than departure if it's next day
    int duration = edge->arriveTS - edge->departTS;

    // If negative (next day arrival), add 24 hours
    if (duration < 0) {
        duration += 24 * 60;
    }

    return duration;
}

void PathFinder::reset_visualization() {
    exploredPorts.length = 0;
    currentPath.length = 0;
}

bool PathFinder::shouldStop() {
    static int callCount = 0;
    callCount++;
    if (callCount > 1000000) {
        if (verbose) cout << "EMERGENCY STOP: Too many operations" << endl;
        return true;
    }
    return false;
}
