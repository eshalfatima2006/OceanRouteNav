#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

int main()
{
    // Create a window with SFML 3.0.2
    sf::RenderWindow window(sf::VideoMode({800, 600}), "SFML 3.0.2 Test");
    window.setFramerateLimit(60);

    // Create a circle shape
    sf::CircleShape circle(50.f);
    circle.setFillColor(sf::Color::Green);
    circle.setPosition({375.f, 275.f});

    // Create a rectangle
    sf::RectangleShape rectangle({100.f, 50.f});
    rectangle.setFillColor(sf::Color::Red);
    rectangle.setPosition({100.f, 100.f});

    // Create text (requires a font)
    sf::Font font;
    sf::Text text(font);
    bool fontLoaded = false;
    
    // Try to load a font (update path as needed)
    if (font.openFromFile("arial.ttf")) {
        text.setString("SFML 3.0.2 Works!");
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setPosition({300.f, 50.f});
        fontLoaded = true;
    }

    // Variables for movement
    sf::Vector2f velocity(2.f, 2.f);

    // Main loop
    while (window.isOpen())
    {
        // Handle events
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }

        // Move circle and bounce off walls
        circle.move(velocity);
        sf::Vector2f pos = circle.getPosition();
        
        if (pos.x <= 0 || pos.x >= 700.f)
            velocity.x = -velocity.x;
        if (pos.y <= 0 || pos.y >= 500.f)
            velocity.y = -velocity.y;

        // Clear, draw, display
        window.clear(sf::Color::Black);
        window.draw(rectangle);
        window.draw(circle);
        if (fontLoaded)
            window.draw(text);
        window.display();
    }

    return 0;
}