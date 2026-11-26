#pragma once

#include "Types.hpp"

#include <deque>
#include <random>
#include <unordered_set>
#include <vector>

// Un serpente nel mondo infinito.
struct Snake
{
    std::deque<Cell> body;     // body.front() = testa
    std::deque<Cell> prevBody; // posizione al passo precedente (per l'interpolazione grafica)
    Direction dir = Direction::Right;
    std::deque<Direction> inputQueue; // svolte prenotate dal giocatore (max 3)
    bool alive = true;
    bool isCpu = false;
    int skin = 0;
    int score = 0;
    float respawnTimer = 0.0f; // solo CPU: tempo prima di ricomparire

    Cell head() const { return body.front(); }
};

// Eventi prodotti dal Model e consumati dalla View (particelle, messaggi).
struct GameEvent
{
    enum class Type
    {
        AppleEaten,
        SnakeDied,
        SnakeRespawned
    };

    Type type;
    Cell cell;
    int snakeIndex;
};

// Stato del gioco: mappa infinita (stile agar.io), mele sparse, uno o due serpenti.
// Non dipende da raylib.
class SnakeModel
{
public:
    SnakeModel();

    // Partita vera: serpente 0 = giocatore, serpente 1 = CPU (solo in VersusCpu)
    void startGame(GameMode mode, int playerSkin, int cpuSkin);
    // Partita "dimostrativa" che gira dietro ai menu: tutti i serpenti sono guidati dal computer
    void startDemo(int snakeCount, int skinA, int skinB);

    void setSkin(int snakeIndex, int skin);
    void queueDirection(int snakeIndex, Direction d);

    void step();                 // avanza tutti i serpenti di una cella
    void updateTimers(float dt); // respawn della CPU

    const std::vector<Snake> &getSnakes() const { return snakes_; }
    const std::unordered_set<Cell, CellHash> &getApples() const { return apples_; }
    const std::vector<GameEvent> &getEvents() const { return events_; }
    void clearEvents() { events_.clear(); }

    GameMode getMode() const { return mode_; }
    bool isDemo() const { return demo_; }
    bool isPlayerDead() const;
    int snakeCount() const { return static_cast<int>(snakes_.size()); }

private:
    Snake makeSnake(Cell start, Direction dir, int skin, bool cpu) const;
    void killSnake(int index);
    void respawnSnake(int index);
    Direction chooseCpuDirection(int index) const;
    int floodFillArea(Cell start, const std::unordered_set<Cell, CellHash> &blocked, int limit) const;
    bool isOccupied(Cell c, int ignoreSnake = -1) const;
    void maintainApples();
    bool findFreeCellNear(Cell center, int minDist, int maxDist, Cell &out);

    std::vector<Snake> snakes_;
    std::unordered_set<Cell, CellHash> apples_;
    std::vector<GameEvent> events_;
    GameMode mode_ = GameMode::SinglePlayer;
    bool demo_ = false;

    mutable std::mt19937 rng_;
};
