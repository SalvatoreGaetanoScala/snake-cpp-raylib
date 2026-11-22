#pragma once

#include <deque>
#include <random>
#include <utility>

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

struct Cell
{
    int x = 0;
    int y = 0;

    bool operator==(const Cell &other) const
    {
        return x == other.x && y == other.y;
    }
};

class SnakeModel
{
public:
    SnakeModel(int gridWidth, int gridHeight);

    void reset();
    void setDirection(Direction newDir);
    void step(); // Avanza di una singola cella

    int getGridWidth() const { return gridWidth_; }
    int getGridHeight() const { return gridHeight_; }
    const std::deque<Cell> &getSnake() const { return snake_; }
    Cell getFood() const { return food_; }
    int getScore() const { return score_; }
    bool isGameOver() const { return gameOver_; }

private:
    bool isOpposite(Direction a, Direction b) const;
    bool isOccupiedBySnake(const Cell &cell) const;
    void spawnFood();

    int gridWidth_;
    int gridHeight_;

    std::deque<Cell> snake_;
    Direction currentDir_ = Direction::Right;
    Direction nextDir_ = Direction::Right;

    Cell food_{0, 0};
    int score_ = 0;
    bool gameOver_ = false;

    std::mt19937 rng_;
};
