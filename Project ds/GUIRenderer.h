#pragma once
#ifndef GUI_RENDERER_H
#define GUI_RENDERER_H

#include <SFML/Graphics.hpp>
#include "CommonStructures.h"
#include "BookingSystem.h"

// Rendering functions for map visualization
void drawGradientBackground(sf::RenderWindow& win, int width, int height);

void drawPort(sf::RenderWindow& win, PortNode* p, sf::Font& font,
    bool highlight = false, bool isOnPath = false);

void drawEdge(sf::RenderWindow& win, PortNode* a, PortNode* b,
    RouteEdge* e, bool highlight = false);

void drawAnimatedPath(sf::RenderWindow& win, Graph& g, DynArray<int>& path,
    float animationProgress, sf::Color color = sf::Color::Green);

void drawDockQueue(sf::RenderWindow& win, PortNode* port, sf::Font& font);

// UI Panel drawing functions
void drawInfoPanel(sf::RenderWindow& win, sf::Font& font, const std::string& info,
    float x, float y, float width, float height);

void drawAlgorithmStatus(sf::RenderWindow& win, sf::Font& font,
    bool algorithmRunning, const std::string& algorithmName,
    int exploredCount, int pathLength, float algorithmTime,
    float animationProgress, int windowWidth);

void drawBookingInfo(sf::RenderWindow& win, BookingSystem& bookingSystem,
    Graph& graph, sf::Font& font, int windowWidth);

// Utility color function
sf::Color routeColorByCost(int cost);

#endif // GUI_RENDERER_H
