#include "GUIInputHandler.h"
#include <cmath>

float pointSegmentDistance(float px, float py, float x1, float y1, float x2, float y2) {
    float vx = x2 - x1;
    float vy = y2 - y1;
    float wx = px - x1;
    float wy = py - y1;
    float l2 = vx * vx + vy * vy;
    if (l2 == 0.0f) return sqrtf((px - x1) * (px - x1) + (py - y1) * (py - y1));
    float t = (wx * vx + wy * vy) / l2;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    float projx = x1 + t * vx;
    float projy = y1 + t * vy;
    float dx = px - projx;
    float dy = py - projy;
    return sqrtf(dx * dx + dy * dy);
}

RouteEdge* find_edge_near(Graph& graph, float mx, float my, PortNode** outFrom, float threshold) {
    RouteEdge* best = nullptr;
    PortNode* bestFrom = nullptr;
    float bestDist = threshold;

    for (int i = 0; i < graph.portsArr.size(); ++i) {
        PortNode* p = graph.portsArr[i];
        if (!p) continue;
        RouteEdge* e = p->edges;
        while (e) {
            PortNode* dest = graph.get_port_by_id(e->destPortID);
            if (dest) {
                float d = pointSegmentDistance(mx, my, p->x, p->y, dest->x, dest->y);
                if (d < bestDist) {
                    bestDist = d;
                    best = e;
                    bestFrom = p;
                }
            }
            e = e->next;
        }
    }
    if (outFrom) *outFrom = bestFrom;
    return best;
}

int getClickedPortID(Graph& graph, sf::Vector2f mousePos, float radius) {
    int best = -1;
    float r2 = radius * radius;

    for (int i = 0; i < graph.portsArr.size(); ++i) {
        PortNode* p = graph.portsArr[i];
        if (!p) continue;
        float dx = mousePos.x - p->x;
        float dy = mousePos.y - p->y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= r2) {
            best = p->id;
            break;
        }
    }
    return best;
}