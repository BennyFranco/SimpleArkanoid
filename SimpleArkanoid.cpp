#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

constexpr int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.0f};

struct Ball
{
    // SFML class that defines a circle shape
    CircleShape shape;

    // Ball constructor
    // mX is the x coordinate
    // mY is the y coordinate
    Ball(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setRadius(ballRadius);
        shape.setFillColor(Color::Red);
        shape.setOrigin(ballRadius, ballRadius);
    }
};

int main()
{
    // Ball instance
    Ball ball{windowWidth / 2, windowHeight / 2};

    RenderWindow window{{windowWidth, windowHeight}, "Simple Arkanoid"};
    window.setFramerateLimit(60);

    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::KeyPressed)
            {
                if (event.key.code == Keyboard::Escape)
                {
                    window.close();
                }
            }
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(Color::Black);
        window.display();
    }

    return 0;
}