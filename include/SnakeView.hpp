#pragma once

#include "SnakeModel.hpp"

class SnakeView
{
public:
    SnakeView(int cellSize, int hudHeight, const char *title);
    ~SnakeView();

    bool shouldClose() const;
    void render(const SnakeModel &model) const;

private:
    int cellSize_;
    int hudHeight_;
};
