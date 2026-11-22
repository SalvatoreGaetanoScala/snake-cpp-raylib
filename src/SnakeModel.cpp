#include "SnakeModel.hpp"

#include <vector>

SnakeModel::SnakeModel(int gridWidth, int gridHeight)
    : gridWidth_(gridWidth),
      gridHeight_(gridHeight),
      rng_(std::random_device{}())
{
    reset();
}

void SnakeModel::reset()
{
    snake_.clear();
    int startX = gridWidth_ / 2;
    int startY = gridHeight_ / 2;

    snake_.push_back({startX, startY});
    snake_.push_back({startX - 1, startY});
    snake_.push_back({startX - 2, startY});

    currentDir_ = Direction::Right;
    nextDir_ = Direction::Right;
    score_ = 0;
    gameOver_ = false;

    spawnFood();
}

void SnakeModel::setDirection(Direction newDir)
{
    // Evita inversioni a 180° nello stesso tick
    if (!isOpposite(currentDir_, newDir))
    {
        nextDir_ = newDir;
    }
}

bool SnakeModel::isOpposite(Direction a, Direction b) const
{
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

bool SnakeModel::isOccupiedBySnake(const Cell &cell) const
{
    for (const auto &segment : snake_)
    {
        if (segment == cell)
        {
            return true;
        }
    }
    return false;
}

void SnakeModel::spawnFood()
{
    std::vector<Cell> freeCells;
    freeCells.reserve(static_cast<size_t>(gridWidth_ * gridHeight_));

    for (int y = 0; y < gridHeight_; ++y)
    {
        for (int x = 0; x < gridWidth_; ++x)
        {
            Cell candidate{x, y};
            if (!isOccupiedBySnake(candidate))
            {
                freeCells.push_back(candidate);
            }
        }
    }

    if (freeCells.empty())
    {
        gameOver_ = true;
        return;
    }

    std::uniform_int_distribution<size_t> dist(0, freeCells.size() - 1);
    food_ = freeCells[dist(rng_)];
}

void SnakeModel::step()
{
    if (gameOver_)
    {
        return;
    }

    currentDir_ = nextDir_;
    Cell newHead = snake_.front();

    switch (currentDir_)
    {
    case Direction::Up:
        newHead.y -= 1;
        break;
    case Direction::Down:
        newHead.y += 1;
        break;
    case Direction::Left:
        newHead.x -= 1;
        break;
    case Direction::Right:
        newHead.x += 1;
        break;
    }

    // Collisione con i bordi
    if (newHead.x < 0 || newHead.x >= gridWidth_ ||
        newHead.y < 0 || newHead.y >= gridHeight_)
    {
        gameOver_ = true;
        return;
    }

    bool willEat = (newHead == food_);

    // Se non mangia, la coda si sposta liberando l'ultima cella
    // Se mangia, la coda resta dov'è (il serpente si allunga)
    if (!willEat)
    {
        snake_.pop_back();
    }

    // Collisione con il corpo
    if (isOccupiedBySnake(newHead))
    {
        gameOver_ = true;
        return;
    }

    snake_.push_front(newHead);

    if (willEat)
    {
        score_ += 10;
        spawnFood();
    }
}
