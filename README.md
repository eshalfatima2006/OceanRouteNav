**OceanRoute Nav - Maritime Navigation System**
**What is this?**
A maritime route planning system that helps find optimal shipping routes between ports. Built with C++ and SFML for visualization.

**Features**
Find shortest/cheapest routes using Dijkstra and A* algorithms

Book cargo routes with layover calculations

View port congestion (waiting queues)

Filter routes by shipping company

Interactive map with animated ship movement

**Requirements**
Visual Studio 2019/2022

SFML 2.6.2

**Setup Instructions**
1. Install SFML
Download SFML 2.6.2 from: https://www.sfml-dev.org/download/sfml/2.6.2/

Extract to C:\SFML-2.6.2 (or any location)

2. Create Visual Studio Project
Open Visual Studio → Create New Project → Empty C++ Project

Add all .h and .cpp files to your project

3. Configure SFML
Right-click project → Properties:

C/C++ → General → Additional Include Directories:

text
C:\SFML-2.6.2\include
Linker → General → Additional Library Directories:

text
C:\SFML-2.6.2\lib
Linker → Input → Additional Dependencies:

text
sfml-graphics.lib
sfml-window.lib
sfml-system.lib
4. Copy SFML DLLs
Copy these files from C:\SFML-2.6.2\bin to your project's Debug/Release folder:

sfml-graphics-2.dll

sfml-window-2.dll

sfml-system-2.dll

5. Place Data Files
Ensure these files are in the same folder as your .exe:

Routes.txt (shipping route data)

PortCharges.txt (port fees)

6. Build & Run
Press F5 to build and run

**How to Use**
**Basic Controls**
Enter origin and destination on the terminal

Click on a route - View voyage details

Hover over route - See company, cost, and time

**Booking a Route**
Click origin port

Click destination port

Enter ship name and cargo type in console

System shows route with cost and time

**Finding Optimal Path**

Choose Dijkstra or A* algorithm using 'D' or 'A' key respectively.

Choose optimize for Cost or Time

Watch algorithm explore ports (highlighted in real-time)

**Applying Filters**
Click Preferences ('P')

Select preferred shipping companies

Add ports to avoid

Set max cost or time limits

**Data File Formats**
Routes.txt (space-separated):

text
Origin Destination Date Departure Arrival Cost Company
Karachi Dubai 15/06/2024 08:00 18:00 8500 MaerskLine
**PortCharges.txt (space-separated):**

text
PortName DailyCharge
Singapore 2000
Karachi 750
**Troubleshooting**
Problem	Solution
Missing SFML headers	Check include paths in project properties
Linker errors	Verify library paths and dependency list
Missing DLLs	Copy SFML DLLs to executable folder
No routes found	Check Routes.txt format and file location
Ports not visible	Verify port coordinates in PortLayout.cpp
**Project Structure**
**File	Purpose**
CommonStructures.h	Graph, PortNode, RouteEdge, DynArray, Hash Table
PathFinder.cpp	Dijkstra & A* algorithms
BookingSystem.cpp	Route booking, multi-leg journeys
DockQueue.cpp	Per-company docking queues
GUIRenderer.cpp	SFML drawing functions
GUIInputHandler.cpp	Mouse click detection
DataLoader.cpp	File parsing
**Demo Scenarios to Try**
**Scenario 1: Find cheapest route from Karachi to Singapore**

Select Dijkstra → Optimize Cost → Click Find Path

**Scenario 2: Book a route with layover**

Click Karachi → Click Rotterdam → Enter ship name → System finds connecting route

**Scenario 3: Filter by company**

Preferences → Add "MaerskLine" → Only Maersk routes shown

**Scenario 4: View port congestion**

Click Singapore port → Shows Q: waiting ships, D: docked ships

**Notes**
No STL containers used (custom implementations per project requirements)

All data structures (DynArray, SimpleHash, PriorityQueue, linked lists) implemented from scratch

Algorithms provide visual feedback during execution
