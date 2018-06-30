#include <math.h>
#include <chrono>

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

using FrameTime = float;

constexpr int windowWidth{800}, windowHeight{600};
constexpr float ballRadius{10.0f}, ballVelocity{0.5f};
constexpr float paddleWidth{60.0f}, paddleHeight{20.0f}, paddleVelocity{0.6f};

// Bricks
constexpr float blockWidth{60.f}, blockHeight{20.f};
constexpr int countBlockX{11}, countBlockY{4};

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
    void Update(FrameTime pFrameTime)
    {
        shape.move(velocity * pFrameTime);

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

    float x()       const noexcept { return shape.getPosition().x; }
    float y()       const noexcept { return shape.getPosition().y; }
    float left()    const noexcept { return x() - shape.getRadius(); }
    float right()   const noexcept { return x() + shape.getRadius(); }
    float top()     const noexcept { return y() - shape.getRadius(); }
    float bottom()  const noexcept { return y() + shape.getRadius(); }
};

struct Rectangle
{
    RectangleShape shape;

    float x()       const noexcept { return shape.getPosition().x; }
    float y()       const noexcept { return shape.getPosition().y; }
    float left()    const noexcept { return x() - shape.getSize().x / 2.f; }
    float right()   const noexcept { return x() + shape.getSize().x / 2.f; }
    float top()     const noexcept { return y() - shape.getSize().y / 2.f; }
    float bottom()  const noexcept { return y() + shape.getSize().y / 2.f; }
};

struct Paddle : public Rectangle
{
    Vector2f velocity;

    Paddle(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({paddleWidth, paddleHeight});
        shape.setFillColor(Color::Red);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    void Update(FrameTime pFrameTime)
    {
        shape.move(velocity * pFrameTime);

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
};

struct Brick : public Rectangle
{
    // With this check if is destroyed or not.
    bool destroyed{false};

    Brick(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({blockWidth, blockHeight});
        shape.setFillColor(Color::Yellow);
        shape.setOrigin(blockWidth / 2.f, blockHeight / 2.f);
    }
};

// Using to check the colliding of two shapes.
template <class T1, class T2>
bool isIntersecting(T1 &mA, T2 &mB)
{
    return mA.right() >= mB.left() && mA.left() <= mB.right() && mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
}

// Testing the paddle and ball collision
void testCollision(Paddle &mPaddle, Ball &mBall)
{
    // If not collision, return.
    if (!isIntersecting(mPaddle, mBall))
        return;

    // Otherwise change velocity (push ball to upwards).
    mBall.velocity.y = -ballVelocity;

    // and dependes of the paddle position.
    if (mBall.x() < mPaddle.x())
        mBall.velocity.x = -ballVelocity;
    else
        mBall.velocity.x = ballVelocity;
}

// Overriding testCollision method for bricks and ball
void testCollision(Brick &mBrick, Ball &mBall)
{
    // If not collision, return.
    if (!isIntersecting(mBrick, mBall))
        return;

    // Otherwise destroy the brick!
    mBrick.destroyed = true;

    // Calculate intersections
    float overlapLeft{mBall.right() - mBrick.left()};
    float overlapRight{mBrick.right() - mBall.left()};
    float overlapTop{mBall.bottom() - mBrick.top()};
    float overlapBottom{mBrick.bottom() - mBall.top()};

    bool ballFromLeft(abs(overlapLeft) < abs(overlapRight));
    bool ballFromTop(abs(overlapTop) < abs(overlapBottom));

    float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
    float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

    if (abs(minOverlapX) < abs(minOverlapY))
        mBall.velocity.x = ballFromLeft ? -ballVelocity : ballVelocity;
    else
        mBall.velocity.y = ballFromTop ? -ballFromTop : ballVelocity;
}

int main()
{
    // Ball instance
    Ball ball{windowWidth / 2, windowHeight / 2};
    Paddle paddle{windowWidth / 2, windowHeight - 50};

    vector<Brick> bricks;

    for (int iX{0}; iX < countBlockX; ++iX)
    {
        for (int iY{0}; iY < countBlockY; ++iY)
        {
            bricks.emplace_back((iX + 1) * (blockWidth + 3) + 22, (iY + 2) * (blockHeight + 3));
        }
    }

    RenderWindow window{{windowWidth, windowHeight}, "Simple Arkanoid"};

    FrameTime lastFrametime{0.f};
    window.setFramerateLimit(60);

    while (window.isOpen())
    {
        // Start of time interval
        auto timePoint1(chrono::high_resolution_clock::now());

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
        ball.Update(lastFrametime);

        // Update the paddle.
        paddle.Update(lastFrametime);

        testCollision(paddle, ball);

        for (auto &brick : bricks)
            testCollision(brick, ball);

        bricks.erase(remove_if(begin(bricks), end(bricks),
                               [](const Brick &mBrick) { return mBrick.destroyed; }),
                     end(bricks));

        // Drawing All.
        window.draw(ball.shape);
        window.draw(paddle.shape);

        // Draw Bricks
        for (auto &brick : bricks)
            window.draw(brick.shape);

        // Displaying the window.
        window.display();

        // End of interval
        auto timePoint2(chrono::high_resolution_clock::now());
        
        // Calculate the elapsed time in milliseconds
        // substract two std::chrono::time_point objects
        // return a std::chrono::duration object, wich
        // represent a period.
        auto elapsedTime(timePoint2 - timePoint1);
        
        // Convert a duration to float using
        // the safe cast function chrono::duration_cast
        FrameTime ft{chrono::duration_cast<chrono::duration<float, milli>>(elapsedTime).count()};

        lastFrametime = ft;
        // We can approximate the fps by dividing 1.f by the elapsed seconds
        // calculated by convert ft to seconds.
        auto ftSeconds(ft/1000.f);
        auto fps(1.f/ftSeconds);

        window.setTitle("FT: " + to_string(ft) + "\t FPS: " + to_string(fps));
    }

    return 0;
}