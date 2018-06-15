#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

constexpr int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.0f}, ballVelocity{8.0f};
constexpr float paddleWidth{60.0f}, paddleHeight{20.0f}, paddleVelocity{6.0f};

struct Ball
{
    // SFML class that defines a circle shape
    CircleShape shape;

    // Velocity of the ball is a vector2d value in negative to move the ball at top and left of the screen in the first quadrant.
    Vector2f velocity{-ballVelocity, -ballVelocity};
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

    // Updates the Ball position by velocity.
    void Update()
    {
        shape.move(velocity);

        // Control de limits of the ball in the screen
        if (left() < 0)
            velocity.x = ballVelocity;

        else if (right() > windowWidth)
            velocity.x = -ballVelocity;

        if (top() < 0)
            velocity.y = ballVelocity;
        else if (bottom() > windowHeight)
            velocity.y = -ballVelocity;
    }

    float x() { return shape.getPosition().x; }
    float y() { return shape.getPosition().y; }
    float left() { return x() - shape.getRadius(); }
    float right() { return x() + shape.getRadius(); }
    float top() { return y() - shape.getRadius(); }
    float bottom() { return y() + shape.getRadius(); }
};

struct Paddle
{
    RectangleShape shape;
    Vector2f velocity;

    Paddle(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({paddleWidth, paddleHeight});
        shape.setFillColor(Color::Red);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    void Update()
    {
        shape.move(velocity);

        if (Keyboard::isKeyPressed(Keyboard::Key::Left) && left() > 0)
        {
            velocity.x = -paddleVelocity;
        }
        else if (Keyboard::isKeyPressed(Keyboard::Key::Right) && right() < windowWidth)
        {
            velocity.x = paddleVelocity;
        }
        else
        {
            velocity.x = 0;
        }
    }

    float x() { return shape.getPosition().x; }
    float y() { return shape.getPosition().y; }
    float left() { return x() - shape.getSize().x / 2.f; }
    float right() { return x() + shape.getSize().x / 2.f; }
    float top() { return y() - shape.getSize().y / 2.f; }
    float bottom() { return y() + shape.getSize().y / 2.f; }
};

// Using to check the colliding of two shapes.
template <class T1, class T2>
bool isIntersecting(T1& mA, T2& mB)
{
    return mA.right() >= mB.left() && mA.left() <= mB.right() && mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
}

// Testing the paddle and ball collision
void testCollision(Paddle &mPaddle, Ball &mBall)
{
    // If not collision, return.
    if(!isIntersecting(mPaddle, mBall)) return;

    // Otherwise change velocity (push ball to upwards).
    mBall.velocity.y = -ballVelocity;

    // and dependes of the paddle position.
    if(mBall.x() < mPaddle.x()) mBall.velocity.x = -ballVelocity;    
    else  mBall.velocity.x = ballVelocity;  
}

int main()
{
    // Ball instance
    Ball ball{windowWidth / 2, windowHeight / 2};
    Paddle paddle{windowWidth / 2, windowHeight - 50};

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

        // Clear window, for some reason I need to do this after events in MacOS.
        window.clear(Color::Black);

        // Update the ball position every loop.
        ball.Update();

        // Update the paddle.
        paddle.Update();

        testCollision(paddle, ball);

        // Drawing All.
        window.draw(ball.shape);
        window.draw(paddle.shape);

        // Displaying the window.
        window.display();
    }

    return 0;
}