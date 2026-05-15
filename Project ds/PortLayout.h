#pragma once
#ifndef PORT_LAYOUT_H
#define PORT_LAYOUT_H

#include "CommonStructures.h"
#include <string>

// Port coordinate mapping structure
struct PortCoordEntry {
    const char* name;
    float x, y;
};

// Port positioning functions
PortCoordEntry* createPortCoordinates(int& count);

void assign_port_positions(Graph& g, int width, int height);

// String utility
std::string toLowerStr(const std::string& s);

#endif // PORT_LAYOUT_H