#include "GUIRenderer.h"
#include "MathUtilities.h"
#include <sstream>
#include <cmath>
#include <SFML/Graphics.hpp> 

using namespace std;

// New function to draw a colored triangle indicator
void drawTriangleIndicator(sf::RenderWindow& win, float x, float y, float size, sf::Color fillColor, sf::Color outlineColor) {
    sf::ConvexShape arrow(3);

    // Define the triangle points 
    // Points relative to the center (x, y)
    arrow.setPoint(0, sf::Vector2f(x, y - size));         // Top point
    arrow.setPoint(1, sf::Vector2f(x - size, y + size));  // Bottom left point
    arrow.setPoint(2, sf::Vector2f(x + size, y + size));  // Bottom right point

    arrow.setFillColor(fillColor);
    arrow.setOutlineColor(outlineColor);
    arrow.setOutlineThickness(1.0f);

    win.draw(arrow);
}


sf::Color routeColorByCost(int cost) {
    int c = mini(255, cost / 150);
    int r = mini(255, c * 2);
    int g = maxi(0, 220 - c);
    int b = 80;
    return sf::Color((sf::Uint8)r, (sf::Uint8)g, (sf::Uint8)b, 230);
}

void drawGradientBackground(sf::RenderWindow& win, int width, int height) {
    sf::VertexArray gradient(sf::Quads, 4);
    gradient[0].position = sf::Vector2f(0, 0);
    gradient[1].position = sf::Vector2f(width, 0);
    gradient[2].position = sf::Vector2f(width, height);
    gradient[3].position = sf::Vector2f(0, height);

    gradient[0].color = sf::Color(10, 10, 40);
    gradient[1].color = sf::Color(20, 20, 60);
    gradient[2].color = sf::Color(5, 5, 30);
    gradient[3].color = sf::Color(15, 15, 50);

    win.draw(gradient);
}

void drawPort(sf::RenderWindow& win, PortNode* p, sf::Font& font, bool highlight, bool isOnPath) {
    if (!p) return;

    sf::Color fillColor;
    sf::Color outlineColor;
    float radius;

    if (isOnPath) {
        radius = 10.0f;
        fillColor = sf::Color(255, 215, 0, 255);
        outlineColor = sf::Color::Black;
    }
    else if (highlight) {
        radius = 10.0f;
        fillColor = sf::Color::Yellow;
        outlineColor = sf::Color::Red;
    }
    else {
        radius = 7.0f;
        fillColor = sf::Color(70, 130, 255, 255);
        outlineColor = sf::Color::White;
    }

    sf::CircleShape circ(radius);
    circ.setOrigin(radius, radius);
    circ.setPosition(p->x, p->y);
    circ.setFillColor(fillColor);
    circ.setOutlineColor(outlineColor);
    circ.setOutlineThickness(2.0f);
    win.draw(circ);

    sf::Text t;
    t.setFont(font);
    t.setCharacterSize(13);
    t.setString(p->name);
    t.setFillColor(sf::Color::Black);
    t.setStyle(sf::Text::Bold);

    sf::FloatRect textBounds = t.getLocalBounds();
    t.setOrigin(textBounds.width / 2.0f, textBounds.height / 2.0f);
    t.setPosition(p->x, p->y + radius + 15);

    sf::RectangleShape textBg(sf::Vector2f(textBounds.width + 8, textBounds.height + 4));
    textBg.setOrigin(textBounds.width / 2.0f + 4, textBounds.height / 2.0f + 2);
    textBg.setPosition(p->x, p->y + radius + 15);
    textBg.setFillColor(sf::Color(255, 255, 255, 220));
    textBg.setOutlineColor(sf::Color::Black);
    textBg.setOutlineThickness(1.0f);
    win.draw(textBg);

    win.draw(t);
}

void drawEdge(sf::RenderWindow& win, PortNode* a, PortNode* b, RouteEdge* e, bool highlight) {
    if (!a || !b || !e) return;

    if (highlight) {
        sf::Vector2f dir(b->x - a->x, b->y - a->y);
        float length = sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length <= 1.0f) return;

        sf::RectangleShape lineShape(sf::Vector2f(length, 4.0f));
        lineShape.setPosition(a->x, a->y);
        lineShape.setFillColor(sf::Color::Cyan);

        float angle = atan2(dir.y, dir.x) * 180.0f / 3.14159f;
        lineShape.setRotation(angle);

        win.draw(lineShape);
    }
    else {
        // Calculate direction and length
        float dx = b->x - a->x;
        float dy = b->y - a->y;
        float length = sqrt(dx * dx + dy * dy);
        if (length <= 1.0f) return;

        // Create 3-pixel thick edge
        sf::RectangleShape thickLine(sf::Vector2f(length, 1.0f));
        thickLine.setPosition(a->x, a->y);

        sf::Color col = routeColorByCost(e->cost);
        thickLine.setFillColor(col);

        float angle = atan2(dy, dx) * 180.0f / 3.14159f;
        thickLine.setRotation(angle);

        win.draw(thickLine);
    }
}


void drawAnimatedPath(sf::RenderWindow& win, Graph& g, DynArray<int>& path,
    float animationProgress, sf::Color color) {
    if (path.size() < 2) return;

    float totalLength = 0.0f;
    for (int i = 0; i < path.size() - 1; ++i) {
        PortNode* a = g.get_port_by_id(path[i]);
        PortNode* b = g.get_port_by_id(path[i + 1]);
        if (a && b) {
            float dx = b->x - a->x;
            float dy = b->y - a->y;
            totalLength += sqrt(dx * dx + dy * dy);
        }
    }
    if (totalLength <= 0.0f) return;

    float currentLength = 0.0f;
    for (int i = 0; i < path.size() - 1; ++i) {
        PortNode* a = g.get_port_by_id(path[i]);
        PortNode* b = g.get_port_by_id(path[i + 1]);
        if (!a || !b) continue;

        float dx = b->x - a->x;
        float dy = b->y - a->y;
        float segLen = sqrt(dx * dx + dy * dy);
        float segStartFrac = currentLength / totalLength;
        float segEndFrac = (currentLength + segLen) / totalLength;
        float segAnim = 0.0f;
        if (animationProgress <= segStartFrac) segAnim = 0.0f;
        else if (animationProgress >= segEndFrac) segAnim = 1.0f;
        else segAnim = (animationProgress - segStartFrac) / (segEndFrac - segStartFrac);

        if (segAnim > 0.0f) {
            sf::Vector2f start(a->x, a->y);
            sf::Vector2f end = start + sf::Vector2f(dx, dy) * segAnim;

            float drawnLen = segLen * segAnim;
            if (drawnLen > 1.0f) {
                sf::RectangleShape lineShape(sf::Vector2f(drawnLen, 5.0f));
                lineShape.setPosition(a->x, a->y);
                lineShape.setFillColor(color);
                float angle = atan2(dy, dx) * 180.0f / 3.14159f;
                lineShape.setRotation(angle);
                win.draw(lineShape);
            }

            if (segAnim < 1.0f) {
                sf::CircleShape ship(8.0f);
                ship.setOrigin(8.0f, 8.0f);
                ship.setPosition(end);
                ship.setFillColor(sf::Color::Yellow);
                ship.setOutlineColor(sf::Color::Black);
                ship.setOutlineThickness(2.0f);
                win.draw(ship);

                sf::ConvexShape arrow(3);
                arrow.setPoint(0, sf::Vector2f(-6, -5));
                arrow.setPoint(1, sf::Vector2f(10, 0));
                arrow.setPoint(2, sf::Vector2f(-6, 5));
                arrow.setFillColor(sf::Color::Red);
                arrow.setPosition(end);
                float angle = atan2(dy, dx) * 180.0f / 3.14159f;
                arrow.setRotation(angle);
                win.draw(arrow);
            }
        }

        currentLength += segLen;

        if (i == 0 || segEndFrac <= animationProgress) {
            sf::CircleShape marker(10.0f);
            marker.setOrigin(10.0f, 10.0f);
            marker.setPosition(b->x, b->y);
            marker.setFillColor(sf::Color(255, 215, 0, 180));
            marker.setOutlineColor(sf::Color::Black);
            marker.setOutlineThickness(2.0f);
            win.draw(marker);
        }
    }
}

void drawInfoPanel(sf::RenderWindow& win, sf::Font& font, const string& info,
    float x, float y, float width, float height) {
    sf::RectangleShape panel(sf::Vector2f(width, height));
    panel.setPosition(x, y);
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(2.0f);
    win.draw(panel);

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(14);
    text.setFillColor(sf::Color::White);
    text.setStyle(sf::Text::Bold);

    string wrapped = "";
    string currentLine = "";
    string word = "";
    for (int i = 0; i < (int)info.size(); ++i) {
        char c = info[i];
        if (c == ' ' || c == '\n' || i == (int)info.size() - 1) {
            if (i == (int)info.size() - 1 && c != ' ' && c != '\n') word.push_back(c);
            if (!currentLine.empty()) {
                string test = currentLine + " " + word;
                text.setString(test);
                if (text.getLocalBounds().width > width - 20) {
                    wrapped += currentLine + "\n";
                    currentLine = word;
                }
                else {
                    currentLine = test;
                }
            }
            else {
                currentLine = word;
            }
            word.clear();
            if (c == '\n') {
                wrapped += currentLine + "\n";
                currentLine.clear();
            }
        }
        else {
            word.push_back(c);
        }
    }
    if (!currentLine.empty()) wrapped += currentLine;

    text.setString(wrapped);
    text.setPosition(x + 10, y + 10);
    win.draw(text);
}

void drawAlgorithmStatus(sf::RenderWindow& win, sf::Font& font,
    bool algorithmRunning, const string& algorithmName,
    int exploredCount, int pathLength, float algorithmTime,
    float animationProgress, int windowWidth) {
    if (!algorithmRunning) return;

    ostringstream ss;
    ss << "Algorithm: " << algorithmName << "\n"
        << "Explored Ports: " << exploredCount << "\n"
        << "Path Length: " << pathLength << "\n"
        << "Time: " << algorithmTime << "s\n"
        << "Progress: " << int(animationProgress * 100) << "%";

    drawInfoPanel(win, font, ss.str(), windowWidth - 220, 10, 210, 120);

    sf::RectangleShape progressBg(sf::Vector2f(200, 15));
    progressBg.setPosition(windowWidth - 215, 135);
    progressBg.setFillColor(sf::Color(50, 50, 50));
    progressBg.setOutlineColor(sf::Color::White);
    progressBg.setOutlineThickness(1.0f);
    win.draw(progressBg);

    sf::RectangleShape progressBar(sf::Vector2f(200 * animationProgress, 15));
    progressBar.setPosition(windowWidth - 215, 135);
    progressBar.setFillColor(sf::Color(0, 200, 0));
    win.draw(progressBar);
}

void drawBookingInfo(sf::RenderWindow& win, BookingSystem& bookingSystem,
    Graph& graph, sf::Font& font, int windowWidth) {
    int bookingCount = bookingSystem.getAllBookings().size();
    if (bookingCount == 0) return;

    ostringstream ss;
    ss << "ACTIVE BOOKINGS: " << bookingCount << "\n";
    ss << "Latest: ";

    ShipBooking* latest = &bookingSystem.getAllBookings()[bookingCount - 1];
    ss << latest->shipName << " (" << latest->cargoType << ")\n";
    ss << "Cost: $" << int(latest->totalCost) << "\n";
    ss << "Route: ";

    BookedLeg* leg = latest->route.getFirst();
    int legCount = 0;
    while (leg && legCount < 3) {
        PortNode* from = graph.get_port_by_id(leg->fromPortID);
        if (from) ss << from->name << "?";
        legCount++;
        leg = latest->route.getNext();
    }
    ss << "...";

    drawInfoPanel(win, font, ss.str(), windowWidth - 220, 160, 210, 120);
}

void drawDockQueue(sf::RenderWindow& win, PortNode* port, sf::Font& font) {
    if (!port) return;

    int totalQueue = port->getTotalWaitingShips();
    int totalDocked = port->getTotalDockedShips();

    // Define the color based on queue size
    sf::Color indicatorColor;
    if (totalQueue == 0) {
        indicatorColor = sf::Color::Green;
    }
    else if (totalQueue < 3) {
        indicatorColor = sf::Color::Yellow;
    }
    else {
        indicatorColor = sf::Color::Red;
    }

    // Position and size parameters for the indicator
    float indicatorSize = 8.0f;
    float indicatorX = port->x - 15.0f;
    float indicatorY = port->y - 15.0f;

    // Call the new arrow drawing function instead of the CircleShape code block
    drawTriangleIndicator(win, indicatorX, indicatorY, indicatorSize / 2.0f, indicatorColor, sf::Color::White);

    if (totalQueue > 0) {
        sf::Text queueText;
        queueText.setFont(font);
        queueText.setCharacterSize(10);
        queueText.setString(string("Q:") + to_string(totalQueue));
        queueText.setPosition(port->x - 20.0f, port->y - 25.0f);
        queueText.setFillColor(sf::Color::White);
        queueText.setStyle(sf::Text::Bold);
        win.draw(queueText);
    }

    sf::Text dockText;
    dockText.setFont(font);
    dockText.setCharacterSize(10);
    dockText.setString(string("D:") + to_string(totalDocked));
    dockText.setPosition(port->x + 10.0f, port->y - 25.0f);
    dockText.setFillColor(sf::Color::Cyan);
    win.draw(dockText);
}