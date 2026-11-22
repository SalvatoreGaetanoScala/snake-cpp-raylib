#pragma once

#include "SnakeModel.hpp"
#include "SnakeView.hpp"

class SnakeController
{
public:
    SnakeController(SnakeModel &model, SnakeView &view, float stepIntervalSeconds);

    void run();

private:
    void handleInput();
    void update(float dt);

    SnakeModel &model_;
    SnakeView &view_;
    float stepInterval_;
    float accumulator_ = 0.0f;
};
