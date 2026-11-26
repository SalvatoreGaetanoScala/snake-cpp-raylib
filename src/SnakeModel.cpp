#include "SnakeModel.hpp"

#include <algorithm>
#include <climits>
#include <queue>

namespace
{
constexpr int kStartLength = 4;
constexpr int kAppleRadius = 22;    // raggio (in celle) in cui mantenere le mele attorno a ogni serpente
constexpr int kApplesPerSnake = 38; // quante mele tenere in quel raggio
constexpr int kDespawnRadius = 48;  // oltre questa distanza da tutti i serpenti la mela sparisce
constexpr int kAppleScore = 10;
constexpr float kCpuRespawnDelay = 2.5f;
constexpr float kDemoRespawnDelay = 1.0f;
} // namespace

SnakeModel::SnakeModel()
    : rng_(std::random_device{}())
{
    startDemo(2, 0, 2);
}

Snake SnakeModel::makeSnake(Cell start, Direction dir, int skin, bool cpu) const
{
    Snake s;
    s.dir = dir;
    s.skin = skin;
    s.isCpu = cpu;
    s.alive = true;

    const Direction back = oppositeOf(dir);
    Cell c = start;
    for (int i = 0; i < kStartLength; ++i)
    {
        s.body.push_back(c);
        c = moveCell(c, back);
    }
    s.prevBody = s.body;
    return s;
}

void SnakeModel::startGame(GameMode mode, int playerSkin, int cpuSkin)
{
    mode_ = mode;
    demo_ = false;
    snakes_.clear();
    apples_.clear();
    events_.clear();

    snakes_.push_back(makeSnake({0, 0}, Direction::Right, playerSkin, false));
    if (mode == GameMode::VersusCpu)
    {
        snakes_.push_back(makeSnake({2, 8}, Direction::Left, cpuSkin, true));
    }
    maintainApples();
}

void SnakeModel::startDemo(int snakeCount, int skinA, int skinB)
{
    mode_ = snakeCount >= 2 ? GameMode::VersusCpu : GameMode::SinglePlayer;
    demo_ = true;
    snakes_.clear();
    apples_.clear();
    events_.clear();

    snakes_.push_back(makeSnake({0, 0}, Direction::Right, skinA, true));
    if (snakeCount >= 2)
    {
        snakes_.push_back(makeSnake({2, 8}, Direction::Left, skinB, true));
    }
    maintainApples();
}

void SnakeModel::setSkin(int snakeIndex, int skin)
{
    if (snakeIndex >= 0 && snakeIndex < snakeCount())
    {
        snakes_[snakeIndex].skin = skin;
    }
}

void SnakeModel::queueDirection(int snakeIndex, Direction d)
{
    if (snakeIndex < 0 || snakeIndex >= snakeCount())
    {
        return;
    }
    Snake &s = snakes_[snakeIndex];
    if (!s.alive || s.isCpu)
    {
        return;
    }

    // Si confronta con l'ultima svolta prenotata: cosi' due tasti premuti velocemente
    // (es. su + sinistra) vengono eseguiti entrambi, senza inversioni a 180 gradi.
    const Direction last = s.inputQueue.empty() ? s.dir : s.inputQueue.back();
    if (d == last || areOpposite(d, last))
    {
        return;
    }
    if (s.inputQueue.size() < 3)
    {
        s.inputQueue.push_back(d);
    }
}

bool SnakeModel::isPlayerDead() const
{
    return !demo_ && !snakes_.empty() && !snakes_[0].alive;
}

bool SnakeModel::isOccupied(Cell c, int ignoreSnake) const
{
    for (int i = 0; i < snakeCount(); ++i)
    {
        if (i == ignoreSnake || !snakes_[i].alive)
        {
            continue;
        }
        for (const Cell &seg : snakes_[i].body)
        {
            if (seg == c)
            {
                return true;
            }
        }
    }
    return false;
}

void SnakeModel::step()
{
    const int n = snakeCount();
    if (n == 0)
    {
        return;
    }

    // 1) Decisioni della CPU e svolte del giocatore
    for (int i = 0; i < n; ++i)
    {
        Snake &s = snakes_[i];
        s.prevBody = s.body;
        if (!s.alive)
        {
            continue;
        }
        if (s.isCpu)
        {
            s.dir = chooseCpuDirection(i);
        }
        else if (!s.inputQueue.empty())
        {
            s.dir = s.inputQueue.front();
            s.inputQueue.pop_front();
        }
    }

    // 2) Nuove teste e mele mangiate
    std::vector<Cell> newHead(n);
    std::vector<char> eats(n, 0);
    std::vector<char> dies(n, 0);
    for (int i = 0; i < n; ++i)
    {
        if (!snakes_[i].alive)
        {
            continue;
        }
        newHead[i] = moveCell(snakes_[i].head(), snakes_[i].dir);
        eats[i] = apples_.count(newHead[i]) > 0 ? 1 : 0;
    }

    // 3) Chi non mangia sposta la coda (liberando l'ultima cella)
    for (int i = 0; i < n; ++i)
    {
        if (snakes_[i].alive && !eats[i])
        {
            snakes_[i].body.pop_back();
        }
    }

    // 4) Collisioni: con qualunque corpo, oppure testa contro testa
    for (int i = 0; i < n; ++i)
    {
        if (!snakes_[i].alive)
        {
            continue;
        }
        if (isOccupied(newHead[i]))
        {
            dies[i] = 1;
        }
        for (int j = 0; j < n; ++j)
        {
            if (j != i && snakes_[j].alive && newHead[j] == newHead[i])
            {
                dies[i] = 1;
            }
        }
    }

    // 5) Applica i risultati
    for (int i = 0; i < n; ++i)
    {
        Snake &s = snakes_[i];
        if (!s.alive)
        {
            continue;
        }
        if (dies[i])
        {
            s.body = s.prevBody; // resta fermo dove si e' schiantato
            killSnake(i);
            continue;
        }
        s.body.push_front(newHead[i]);
        if (eats[i])
        {
            apples_.erase(newHead[i]);
            s.score += kAppleScore;
            events_.push_back({GameEvent::Type::AppleEaten, newHead[i], i});
        }
    }

    maintainApples();
}

void SnakeModel::killSnake(int index)
{
    Snake &s = snakes_[index];
    s.alive = false;
    s.inputQueue.clear();
    events_.push_back({GameEvent::Type::SnakeDied, s.head(), index});

    if (s.isCpu)
    {
        // Come in agar.io / slither: il corpo si trasforma in mele
        for (std::size_t k = 0; k < s.body.size(); k += 2)
        {
            if (!isOccupied(s.body[k], index))
            {
                apples_.insert(s.body[k]);
            }
        }
        s.body.clear();
        s.prevBody.clear();
        s.respawnTimer = demo_ ? kDemoRespawnDelay : kCpuRespawnDelay;
    }
}

void SnakeModel::updateTimers(float dt)
{
    for (int i = 0; i < snakeCount(); ++i)
    {
        Snake &s = snakes_[i];
        if (!s.alive && s.isCpu)
        {
            s.respawnTimer -= dt;
            if (s.respawnTimer <= 0.0f)
            {
                respawnSnake(i);
            }
        }
    }
}

void SnakeModel::respawnSnake(int index)
{
    // Ricompare vicino a un altro serpente vivo (di solito il giocatore)
    Cell center{0, 0};
    for (int j = 0; j < snakeCount(); ++j)
    {
        if (j != index && snakes_[j].alive)
        {
            center = snakes_[j].head();
            break;
        }
    }

    std::uniform_int_distribution<int> offset(-16, 16);
    std::uniform_int_distribution<int> dirDist(0, 3);
    for (int attempt = 0; attempt < 300; ++attempt)
    {
        Cell start{center.x + offset(rng_), center.y + offset(rng_)};
        if (chebyshev(start, center) < 9)
        {
            continue;
        }
        const Direction dir = static_cast<Direction>(dirDist(rng_));
        Snake candidate = makeSnake(start, dir, snakes_[index].skin, true);

        bool free = true;
        for (const Cell &c : candidate.body)
        {
            if (isOccupied(c, index))
            {
                free = false;
                break;
            }
        }
        if (!free)
        {
            continue;
        }
        for (const Cell &c : candidate.body)
        {
            apples_.erase(c);
        }
        snakes_[index] = candidate;
        events_.push_back({GameEvent::Type::SnakeRespawned, start, index});
        return;
    }
    snakes_[index].respawnTimer = 0.5f; // riprova fra poco
}

int SnakeModel::floodFillArea(Cell start, const std::unordered_set<Cell, CellHash> &blocked, int limit) const
{
    std::unordered_set<Cell, CellHash> visited;
    visited.reserve(static_cast<std::size_t>(limit) * 2);
    std::queue<Cell> q;
    q.push(start);
    visited.insert(start);
    int count = 0;

    static const Direction dirs[4] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    while (!q.empty() && count < limit)
    {
        Cell c = q.front();
        q.pop();
        ++count;
        for (Direction d : dirs)
        {
            Cell nb = moveCell(c, d);
            if (blocked.count(nb) || visited.count(nb))
            {
                continue;
            }
            visited.insert(nb);
            q.push(nb);
        }
    }
    return count;
}

Direction SnakeModel::chooseCpuDirection(int index) const
{
    const Snake &s = snakes_[index];
    const Cell head = s.head();

    // Celle occupate (la coda di ogni serpente si sposta, quindi non conta)
    std::unordered_set<Cell, CellHash> blocked;
    for (const Snake &other : snakes_)
    {
        if (!other.alive)
        {
            continue;
        }
        for (std::size_t k = 0; k + 1 < other.body.size(); ++k)
        {
            blocked.insert(other.body[k]);
        }
    }

    // Obiettivo: la mela piu' vicina
    bool hasTarget = false;
    Cell target = head;
    int bestDist = INT_MAX;
    for (const Cell &a : apples_)
    {
        int d = manhattan(a, head);
        if (d < bestDist)
        {
            bestDist = d;
            target = a;
            hasTarget = true;
        }
    }

    const int need = std::min(static_cast<int>(s.body.size()) + 8, 120);
    std::uniform_real_distribution<float> jitter(0.0f, 0.6f);

    static const Direction dirs[4] = {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    float bestScore = -1e9f;
    Direction bestDir = s.dir;

    for (Direction d : dirs)
    {
        if (areOpposite(d, s.dir))
        {
            continue;
        }
        const Cell c = moveCell(head, d);
        if (blocked.count(c))
        {
            continue;
        }

        float score = 0.0f;
        if (hasTarget)
        {
            score -= static_cast<float>(manhattan(c, target));
        }

        // Evita di infilarsi in vicoli ciechi
        const int area = floodFillArea(c, blocked, need);
        if (area < need)
        {
            score -= 500.0f + static_cast<float>(need - area) * 5.0f;
        }

        // Evita gli scontri testa contro testa
        for (int j = 0; j < snakeCount(); ++j)
        {
            if (j == index || !snakes_[j].alive)
            {
                continue;
            }
            const Snake &o = snakes_[j];
            if (moveCell(o.head(), o.dir) == c)
            {
                score -= 300.0f;
            }
            else if (manhattan(c, o.head()) <= 1)
            {
                score -= 40.0f;
            }
        }

        if (d == s.dir)
        {
            score += 0.3f;
        }
        score += jitter(rng_);

        if (score > bestScore)
        {
            bestScore = score;
            bestDir = d;
        }
    }
    return bestDir;
}

bool SnakeModel::findFreeCellNear(Cell center, int minDist, int maxDist, Cell &out)
{
    std::uniform_int_distribution<int> dist(-maxDist, maxDist);
    for (int attempt = 0; attempt < 40; ++attempt)
    {
        Cell c{center.x + dist(rng_), center.y + dist(rng_)};
        if (chebyshev(c, center) < minDist || apples_.count(c) || isOccupied(c))
        {
            continue;
        }
        out = c;
        return true;
    }
    return false;
}

void SnakeModel::maintainApples()
{
    std::vector<Cell> heads;
    for (const Snake &s : snakes_)
    {
        if (s.alive)
        {
            heads.push_back(s.head());
        }
    }
    if (heads.empty())
    {
        return;
    }

    // Rimuove le mele troppo lontane da tutti (la mappa e' infinita)
    for (auto it = apples_.begin(); it != apples_.end();)
    {
        bool far = true;
        for (const Cell &h : heads)
        {
            if (chebyshev(*it, h) <= kDespawnRadius)
            {
                far = false;
                break;
            }
        }
        it = far ? apples_.erase(it) : std::next(it);
    }

    // Mantiene una densita' costante di mele attorno a ogni serpente
    for (const Cell &h : heads)
    {
        int count = 0;
        for (const Cell &a : apples_)
        {
            if (chebyshev(a, h) <= kAppleRadius)
            {
                ++count;
            }
        }
        for (int k = count; k < kApplesPerSnake; ++k)
        {
            Cell c;
            if (findFreeCellNear(h, 3, kAppleRadius, c))
            {
                apples_.insert(c);
            }
        }
    }
}
