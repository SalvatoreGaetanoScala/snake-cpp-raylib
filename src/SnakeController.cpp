#include "SnakeController.hpp"

#include "raylib.h"

SnakeController::SnakeController(SnakeModel &model, SnakeView &view, float stepIntervalSeconds)
    : model_(model),
      view_(view),
      stepInterval_(stepIntervalSeconds) {}

void SnakeController::run()
{
    while (!view_.shouldClose())
    {
        float dt = GetFrameTime();
        handleInput();
        update(dt);
        view_.render(model_);
    }
}

void SnakeController::handleInput()
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        model_.setDirection(Direction::Up);
    }
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        model_.setDirection(Direction::Down);
    }
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
    {
        model_.setDirection(Direction::Left);
    }
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
    {
        model_.setDirection(Direction::Right);
    }

    if (IsKeyPressed(KEY_R))
    {
        model_.reset();
        accumulator_ = 0.0f;
    }
}

void SnakeController::update(float dt)
{
    if (model_.isGameOver())
    {
        return;
    }

    accumulator_ += dt;
    while (accumulator_ >= stepInterval_)
    {
        accumulator_ -= stepInterval_;
        model_.step();
    }
}
