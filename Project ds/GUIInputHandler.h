#pragma once
#ifndef GUI_INPUT_HANDLER_H
#define GUI_INPUT_HANDLER_H

#include <SFML/Graphics.hpp>
#include "CommonStructures.h"
#include "PathFinder.h"

// Mouse interaction structures
struct ClickBookingState {
    int originID = -1;
    int destID = -1;
    bool waitingForConfirmation = false;
    bool showBookingResult = false;
    std::string bookingResultMsg = "";
    PathResult tentativePath;
};

// Mouse utility functions
int getClickedPortID(Graph& graph, sf::Vector2f mousePos, float radius = 12.0f);

RouteEdge* find_edge_near(Graph& graph, float mx, float my,
    PortNode** outFrom, float threshold = 6.0f);

float pointSegmentDistance(float px, float py, float x1, float y1,
    float x2, float y2);

#endif // GUI_INPUT_HANDLER_H
