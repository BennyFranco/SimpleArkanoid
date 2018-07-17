#include <math.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>
#include <bitset>
#include <array>
#include <cassert>
#include <type_traits>
#include <functional>

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

// Frametime calculations
constexpr float ftStep{1.f}, ftSlice{1.f};

namespace CompositionArkanoid
{
struct Component;
class Entity;
struct Manager;
struct Game;

using ComponentID = std::size_t;
using Group = std::size_t;

namespace Internal
{
inline ComponentID getUniqueComponentID() noexcept
{
    static ComponentID lastID{0u};
    return lastID++;
}
} // namespace Internal

template <typename T>
inline ComponentID getComponentTypeID() noexcept
{
    // Make sure this function is only called with types that inherit from `Component`.
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
    static ComponentID typeID{Internal::getUniqueComponentID()};
    return typeID;
}

// Max number of componets
constexpr std::size_t maxComponents{32};

// Define a bitset for out components
using ComponentBitset = std::bitset<maxComponents>;

// And also define an array for them.
using ComponentArray = std::array<Component *, maxComponents>;

constexpr std::size_t maxGroups{32};
using GroupBitset = std::bitset<maxGroups>;

struct Component
{
    // Pointer to parent entity
    Entity *entity;

    virtual void init() {}
    virtual void update(float mFT) {}
    virtual void draw() {}

    virtual ~Component() {}
};

class Entity
{
  private:
    Manager &manager;
    // Used to know if the entity is aliver or not
    bool alive{true};

    // Here we store components to allow polymorphism
    std::vector<std::unique_ptr<Component>> components;

    // Arrays to get a component with specific ID and known the existing of a
    // component with specific ID.
    ComponentArray componentArray;
    ComponentBitset componentBitset;

    GroupBitset groupBitset;

  public:
    Entity(Manager &mManager) : manager(mManager) {}

    void update(float mFT)
    {
        for (auto &component : components)
            component->update(mFT);
    }
    void draw()
    {
        for (auto &component : components)
            component->draw();
    }

    bool isAlive() const { return alive; }
    void destroy() { alive = false; }

    // To check if an entity has a component.
    template <typename T>
    bool hasComponent() const
    {
        return componentBitset[getComponentTypeID<T>()];
    }

    bool hasGroup(Group mGroup) const noexcept
    {
        return groupBitset[mGroup];
    }

    void addGroup(Group mGroup) noexcept;
    void delGroup(Group mGroup) noexcept
    {
        groupBitset[mGroup] = false;
    }

    template <typename T, typename... TArgs>
    T &addComponent(TArgs &&... mArgs)
    {
        assert(!hasComponent<T>());

        T *c(new T(std::forward<TArgs>(mArgs)...));
        c->entity = this;
        std::unique_ptr<Component> uPtr{c};
        components.emplace_back(std::move(uPtr));

        componentArray[getComponentTypeID<T>()] = c;
        componentBitset[getComponentTypeID<T>()] = true;

        c->init();

        return *c;
    }

    template <typename T>
    T &getComponent() const
    {
        assert(hasComponent<T>());
        auto ptr(componentArray[getComponentTypeID<T>()]);
        return *static_cast<T *>(ptr);
    }
};

struct Manager
{
  private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::array<std::vector<Entity *>, maxGroups> groupedEntities;

  public:
    void update(float mFT)
    {
        // Clean up dead entities
        entities.erase(remove_if(begin(entities), end(entities),
                                 [](const std::unique_ptr<Entity> &mEntity) {
                                     return !mEntity->isAlive();
                                 }),
                       end(entities));

        for (auto &entity : entities)
            entity->update(mFT);
    }

    void draw()
    {
        for (auto &entity : entities)
            entity->draw();
    }

    void addToGroup(Entity *mEntity, Group mGroup)
    {
        groupedEntities[mGroup].emplace_back(mEntity);
    }

    std::vector<Entity *> &getEntitiesByGroup(Group mGroup)
    {
        return groupedEntities[mGroup];
    }

    void refresh()
    {
        for (auto i(0u); i < maxGroups; i++)
        {
            auto &v(groupedEntities[i]);

            v.erase(
                std::remove_if(std::begin(v), std::end(v),
                               [i](Entity *mEntity) {
                                   return !mEntity->isAlive() || !mEntity->hasGroup(i);
                               }),
                std::end(v));
        }

        entities.erase(
            std::remove_if(std::begin(entities), std::end(entities),
                           [](const std::unique_ptr<Entity> &mEntity) {
                               return !mEntity->isAlive();
                           }),
            std::end(entities));
    }

    Entity &addEntity()
    {
        Entity *e{new Entity{*this}};
        std::unique_ptr<Entity> uPtr{e};
        entities.emplace_back(std::move(uPtr));
        return *e;
    }
};

void Entity::addGroup(Group mGroup) noexcept
{
    groupBitset[mGroup] = true;
    manager.addToGroup(this, mGroup);
}

// Position of the entities in the world
struct CPosition : Component
{
    Vector2f position;

    CPosition() = default;
    CPosition(const Vector2f &mPosition) : position(mPosition) {}

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
};

struct CPhysics : Component
{
    CPosition *cPosition{nullptr};
    Vector2f velocity, halfSize;

    std::function<void(const Vector2f &)> onOutOfBounds;
    CPhysics(const Vector2f &mHalfSize) : halfSize{mHalfSize} {}

    void init() override
    {
        cPosition = &entity->getComponent<CPosition>();
    }

    void update(float mFT) override
    {
        cPosition->position += velocity * mFT;

        if (onOutOfBounds == nullptr)
            return;

        if (left() < 0)
            onOutOfBounds(Vector2f{1.f, 0.f});
        else if (right() > windowWidth)
            onOutOfBounds(Vector2f{-1.f, 0.f});

        if (top() < 0)
            onOutOfBounds(Vector2f{0.f, 1.0f});
        else if (bottom() > windowHeight)
            onOutOfBounds(Vector2f{0.f, -1.f});
    }

    float x() const noexcept { return cPosition->x(); }
    float y() const noexcept { return cPosition->y(); }
    float left() const noexcept { return x() - halfSize.x; }
    float right() const noexcept { return x() + halfSize.x; }
    float top() const noexcept { return y() - halfSize.y; }
    float bottom() const noexcept { return y() + halfSize.y; }
};

struct CCircle : Component
{
    Game *game{nullptr};
    CPosition *cPosition{nullptr};
    CircleShape shape;
    float radius;

    CCircle(Game *mGame, float mRadius) : game{mGame}, radius{mRadius} {}

    void init() override
    {
        cPosition = &entity->getComponent<CPosition>();

        shape.setRadius(radius);
        shape.setFillColor(Color::Red);
        shape.setOrigin(radius, radius);
    }

    void update(float mFT) override
    {
        shape.setPosition(cPosition->position);
    }

    void draw() override;
};

struct CRectangle : Component
{
    Game *game{nullptr};
    CPosition *cPosition{nullptr};
    RectangleShape shape;
    Vector2f size;

    CRectangle(Game *mGame, const Vector2f &mHalfSize) : game{mGame}, size{mHalfSize * 2.f} {}

    void init() override
    {
        cPosition = &entity->getComponent<CPosition>();

        shape.setSize(size);
        shape.setFillColor(Color::Red);
        shape.setOrigin(size.x / 2.f, size.y / 2.f);
    }

    void update(float mFT) override
    {
        shape.setPosition(cPosition->position);
    }

    void draw() override;
};

struct CPaddleControl : Component
{
    CPhysics *cPhysics{nullptr};

    void init() override
    {
        cPhysics = &entity->getComponent<CPhysics>();
    }

    void update(float mFT) override
    {
        if (Keyboard::isKeyPressed(Keyboard::Key::Left) && cPhysics->left() > 0)
        {
            cPhysics->velocity.x = -paddleVelocity;
        }
        else if (Keyboard::isKeyPressed(Keyboard::Key::Right) && cPhysics->right() < windowWidth)
        {
            cPhysics->velocity.x = paddleVelocity;
        }
        else
        {
            cPhysics->velocity.x = 0;
        }
    }
};

// Using to check the colliding of two shapes.
template <class T1, class T2>
bool isIntersecting(T1 &mA, T2 &mB)
{
    return mA.right() >= mB.left() && mA.left() <= mB.right() && mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
}

// Testing the paddle and ball collision
void testCollisionPB(Entity &mPaddle, Entity &mBall)
{
    auto &cpPaddle(mPaddle.getComponent<CPhysics>());
    auto &cpBall(mBall.getComponent<CPhysics>());

    // If not collision, return.
    if (!isIntersecting(cpPaddle, cpBall))
        return;

    // Otherwise change velocity (push ball to upwards).
    cpBall.velocity.y = -ballVelocity;

    // and dependes of the paddle position.
    if (cpBall.x() < cpPaddle.x())
        cpBall.velocity.x = -ballVelocity;
    else
        cpBall.velocity.x = ballVelocity;
}

// Overriding testCollision method for bricks and ball
void testCollisionBB(Entity &mBrick, Entity &mBall)
{
    auto &cpBrick(mBrick.getComponent<CPhysics>());
    auto &cpBall(mBall.getComponent<CPhysics>());

    // If not collision, return.
    if (!isIntersecting(cpBrick, cpBall))
        return;

    // Otherwise destroy the brick!
    mBrick.destroy();

    // Calculate intersections
    float overlapLeft{cpBall.right() - cpBrick.left()};
    float overlapRight{cpBrick.right() - cpBall.left()};
    float overlapTop{cpBall.bottom() - cpBrick.top()};
    float overlapBottom{cpBrick.bottom() - cpBall.top()};

    bool ballFromLeft(abs(overlapLeft) < abs(overlapRight));
    bool ballFromTop(abs(overlapTop) < abs(overlapBottom));

    float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
    float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

    if (abs(minOverlapX) < abs(minOverlapY))
        cpBall.velocity.x = ballFromLeft ? -ballVelocity : ballVelocity;
    else
        cpBall.velocity.y = ballFromTop ? -ballFromTop : ballVelocity;
}

struct Game
{
    enum ArkanoidGroup : std::size_t
    {
        GPaddle,
        GBrick,
        GBall
    };

    RenderWindow window{{windowWidth, windowHeight}, "Simple Arkanoid"};
    // Accumulate the current frametime slice.
    // If the game run fast, it will take some frames before
    // currentSlice >= ftSlice.
    // else if the game run slow, it will take a single frame for
    // currentSlice >= ftSlice * n, where n >= 1.
    FrameTime lastFrametime{0.f}, currentSlice{0.f};
    bool running{false};
    Manager manager;

    Entity &createBall()
    {
        auto &entity(manager.addEntity());
        entity.addComponent<CPosition>(Vector2f{windowWidth / 2, windowHeight / 2});
        entity.addComponent<CPhysics>(Vector2f{ballRadius, ballRadius});
        entity.addComponent<CCircle>(this, ballRadius);

        auto &cPhysics(entity.getComponent<CPhysics>());
        cPhysics.velocity = Vector2f{-ballVelocity, -ballVelocity};
        cPhysics.onOutOfBounds = [&cPhysics](const Vector2f &mSide) {
            if (mSide.x != 0.f)
            {
                cPhysics.velocity.x = std::abs(cPhysics.velocity.x) * mSide.x;
            }

            if (mSide.y != 0.f)
            {
                cPhysics.velocity.y = std::abs(cPhysics.velocity.y) * mSide.y;
            }
        };

        entity.addGroup(ArkanoidGroup::GBall);

        return entity;
    }

    Entity &createBrick(const Vector2f &mPosition)
    {
        Vector2f halfSize{blockWidth / 2.f, blockHeight / 2.f};
        auto &entity(manager.addEntity());

        entity.addComponent<CPosition>(mPosition);
        entity.addComponent<CPhysics>(halfSize);
        entity.addComponent<CRectangle>(this, halfSize);

        entity.addGroup(ArkanoidGroup::GBrick);

        return entity;
    }

    Entity &createPaddle()
    {
        Vector2f halfSize{paddleWidth / 2.f, paddleHeight / 2.f};
        auto &entity(manager.addEntity());

        entity.addComponent<CPosition>(Vector2f{windowWidth / 2, windowHeight - 60.f});
        entity.addComponent<CPhysics>(halfSize);
        entity.addComponent<CRectangle>(this, halfSize);
        entity.addComponent<CPaddleControl>();

        entity.addGroup(ArkanoidGroup::GPaddle);

        return entity;
    }

    Game()
    {
        window.setFramerateLimit(60);
        createPaddle();
        createBall();
        for (int iX{0}; iX < countBlockX; ++iX)
        {
            for (int iY{0}; iY < countBlockY; ++iY)
            {
                createBrick(Vector2f{(iX + 1) * (blockWidth + 3) + 22, (iY + 2) * (blockHeight + 3)});
            }
        }
    }

    void run()
    {
        running = true;

        while (running)
        {
            // Start of time interval
            auto timePoint1(chrono::high_resolution_clock::now());

            inputPhase();
            updatePhase();
            drawPhase();

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
            auto ftSeconds(ft / 1000.f);
            auto fps(1.f / ftSeconds);

            window.setTitle("FT: " + to_string(ft) + "\t FPS: " + to_string(fps));
        }
    }

    void inputPhase()
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
            {
                running = false;
                window.close();
                break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape))
        {
            running = false;
        }
    }

    void updatePhase()
    {
        // Start accumulate frametime
        currentSlice += lastFrametime;

        // If currentSilice is grather or equal to ftSlice we update our game logic
        // and decrease currentSlice until currentSlice becomes less than ftSlice.
        // Ex. if currentSlice is three times as big as ftSlice, we update or
        // game logic three times.
        for (; currentSlice >= ftSlice; currentSlice -= ftSlice)
        {
            /* // Update the ball position every loop.
            ball.Update(ftStep);
            // Update the paddle.
            paddle.Update(ftStep);*/
            manager.refresh();
            manager.update(ftStep);

            auto &paddles(manager.getEntitiesByGroup(GPaddle));
            auto &bricks(manager.getEntitiesByGroup(GBrick));
            auto &balls(manager.getEntitiesByGroup(GBall));

            for (auto &b : balls)
            {
                for (auto &p : paddles)
                    testCollisionPB(*p, *b);
                for (auto &br : bricks)
                    testCollisionBB(*br, *b);
            }
        }
    }

    void drawPhase()
    {
        // Clear window, for some reason I need to do this after events in MacOS.
        window.clear(Color::Black);

        manager.draw();

        // Displaying the window.
        window.display();
    }

    void render(const Drawable &mDrawable)
    {
        window.draw(mDrawable);
    }
};

void CCircle::draw()
{
    game->render(shape);
}

void CRectangle::draw()
{
    game->render(shape);
}
} // namespace CompositionArkanoid

int main()
{
    CompositionArkanoid::Game{}.run();
    return 0;
}
