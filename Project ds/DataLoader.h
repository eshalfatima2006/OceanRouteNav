#pragma once
#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include "CommonStructures.h"
#include <string>

// File loading functions
bool load_port_charges(Graph& g, const std::string& filepath);
bool load_routes(Graph& g, const std::string& filepath);

#endif // DATA_LOADER_H
