#include "SnakeController.hpp"

#include "raylib.h"

#include <algorithm>

namespace
{
constexpr float kDeathDelay = 1.3f; // secondi di "lampeggio" prima del Game Over

int nextSkin(int current, int delta, int skip)
{
    int v = current;
    do
    {
        v = (v + delta + kSkinCount) % kSkinCount;
    } while (v == skip);
    return v;
}

int wrapIndex(int v, int count) { return (v % count + count) % count; }

bool altDown() { return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT); }
} // namespace

SnakeController::SnakeController(SnakeModel &model, SnakeView &view, float stepIntervalSeconds)
    : model_(model),
      view_(view),
      stepInterval_(stepIntervalSeconds) {}

// ---------------------------------------------------------------------------
// Tasti
// ---------------------------------------------------------------------------

bool SnakeController::upPressed() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
bool SnakeController::downPressed() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
bool SnakeController::leftPressed() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
bool SnakeController::rightPressed() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }

bool SnakeController::confirmPressed()
{
    // Alt+Invio e' riservato allo schermo intero
    return (IsKeyPressed(KEY_ENTER) && !altDown()) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
}

bool SnakeController::backPressed() { return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }

// ---------------------------------------------------------------------------
// Loop principale
// ---------------------------------------------------------------------------

void SnakeController::run()
{
    goToMainMenu();
    while (!view_.shouldClose() && !state_.quit)
    {
        const float dt = std::min(GetFrameTime(), 0.1f);
        handleGlobalInput();
        handleScreenInput();
        update(dt);
        view_.render(state_, model_, std::clamp(accumulator_ / stepInterval_, 0.0f, 1.0f));
    }
}

void SnakeController::handleGlobalInput()
{
    // Schermo intero: F11, Alt+Invio (Windows) oppure Ctrl+Cmd+F (macOS)
    const bool macCombo = IsKeyPressed(KEY_F) && (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) &&
                          (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
    if (IsKeyPressed(KEY_F11) || (IsKeyPressed(KEY_ENTER) && altDown()) || macCombo)
    {
        view_.toggleFullscreen();
    }
}

void SnakeController::setScreen(Screen screen)
{
    state_.screen = screen;
    state_.screenTime = 0.0f;
}

void SnakeController::goToMainMenu()
{
    const int a = GetRandomValue(0, kSkinCount - 1);
    const int b = nextSkin(a, GetRandomValue(1, kSkinCount - 1), -1);
    model_.startDemo(2, a, b);
    view_.resetEffects(model_);
    accumulator_ = 0.0f;
    state_.mainIndex = 0;
    setScreen(Screen::MainMenu);
}

void SnakeController::enterColorScreen(Screen screen)
{
    const int count = state_.mode == GameMode::VersusCpu ? 2 : 1;
    if (screen == Screen::ColorCpu && state_.cpuSkin == state_.playerSkin)
    {
        state_.cpuSkin = nextSkin(state_.cpuSkin, 1, state_.playerSkin);
    }
    // Il serpente dimostrativo sullo sfondo mostra in diretta il colore scelto
    if (!model_.isDemo() || model_.snakeCount() != count)
    {
        model_.startDemo(count, state_.playerSkin, state_.cpuSkin);
        view_.resetEffects(model_);
        accumulator_ = 0.0f;
    }
    model_.setSkin(0, state_.playerSkin);
    model_.setSkin(1, state_.cpuSkin);
    setScreen(screen);
}

void SnakeController::startGame()
{
    model_.startGame(state_.mode, state_.playerSkin, state_.cpuSkin);
    view_.resetEffects(model_);
    accumulator_ = 0.0f;
    state_.deathTimer = 0.0f;
    state_.pauseIndex = 0;
    state_.gameOverIndex = 0;
    setScreen(Screen::Playing);
}

void SnakeController::handleScreenInput()
{
    switch (state_.screen)
    {
    case Screen::MainMenu:
        if (upPressed() || downPressed())
        {
            state_.mainIndex = wrapIndex(state_.mainIndex + (upPressed() ? -1 : 1), 2);
        }
        if (confirmPressed())
        {
            if (state_.mainIndex == 0)
            {
                state_.modeIndex = 0;
                setScreen(Screen::ModeMenu);
            }
            else
            {
                state_.quit = true;
            }
        }
        else if (backPressed())
        {
            state_.mainIndex = 1; // come in FF7: "indietro" porta il cursore su Esci
        }
        break;

    case Screen::ModeMenu:
        if (upPressed() || downPressed())
        {
            state_.modeIndex = wrapIndex(state_.modeIndex + (upPressed() ? -1 : 1), 2);
        }
        if (confirmPressed())
        {
            state_.mode = state_.modeIndex == 0 ? GameMode::SinglePlayer : GameMode::VersusCpu;
            enterColorScreen(Screen::ColorPlayer);
        }
        else if (backPressed())
        {
            setScreen(Screen::MainMenu);
            state_.screenTime = 1.0f; // niente animazione di apertura al ritorno
        }
        break;

    case Screen::ColorPlayer:
        if (leftPressed() || rightPressed())
        {
            state_.playerSkin = nextSkin(state_.playerSkin, leftPressed() ? -1 : 1, -1);
            model_.setSkin(0, state_.playerSkin);
        }
        if (confirmPressed())
        {
            if (state_.mode == GameMode::SinglePlayer)
            {
                startGame();
            }
            else
            {
                enterColorScreen(Screen::ColorCpu);
            }
        }
        else if (backPressed())
        {
            setScreen(Screen::ModeMenu);
            state_.screenTime = 1.0f;
        }
        break;

    case Screen::ColorCpu:
        if (leftPressed() || rightPressed())
        {
            state_.cpuSkin = nextSkin(state_.cpuSkin, leftPressed() ? -1 : 1, state_.playerSkin);
            model_.setSkin(1, state_.cpuSkin);
        }
        if (confirmPressed())
        {
            startGame();
        }
        else if (backPressed())
        {
            setScreen(Screen::ColorPlayer);
        }
        break;

    case Screen::Playing:
        if (IsKeyPressed(KEY_ESCAPE))
        {
            state_.pauseIndex = 0;
            setScreen(Screen::Paused);
            break;
        }
        if (upPressed()) model_.queueDirection(0, Direction::Up);
        if (downPressed()) model_.queueDirection(0, Direction::Down);
        if (leftPressed()) model_.queueDirection(0, Direction::Left);
        if (rightPressed()) model_.queueDirection(0, Direction::Right);
        break;

    case Screen::Paused:
        if (upPressed() || downPressed())
        {
            state_.pauseIndex = wrapIndex(state_.pauseIndex + (upPressed() ? -1 : 1), 2);
        }
        if (confirmPressed())
        {
            if (state_.pauseIndex == 0)
            {
                setScreen(Screen::Playing);
            }
            else
            {
                goToMainMenu();
            }
        }
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            setScreen(Screen::Playing);
        }
        break;

    case Screen::GameOver:
        if (upPressed() || downPressed())
        {
            state_.gameOverIndex = wrapIndex(state_.gameOverIndex + (upPressed() ? -1 : 1), 2);
        }
        if (confirmPressed())
        {
            if (state_.gameOverIndex == 0)
            {
                startGame();
            }
            else
            {
                goToMainMenu();
            }
        }
        break;
    }
}

void SnakeController::update(float dt)
{
    state_.screenTime += dt;

    bool simulate = true;
    if (state_.screen == Screen::Paused || state_.screen == Screen::GameOver)
    {
        simulate = false;
    }
    else if (state_.screen == Screen::Playing && model_.isPlayerDead())
    {
        simulate = false;
        state_.deathTimer += dt;
        if (state_.deathTimer >= kDeathDelay)
        {
            state_.gameOverIndex = 0;
            setScreen(Screen::GameOver);
        }
    }

    if (simulate)
    {
        model_.updateTimers(dt);
        accumulator_ += dt;
        while (accumulator_ >= stepInterval_)
        {
            accumulator_ -= stepInterval_;
            model_.step();
            if (state_.screen == Screen::Playing && model_.isPlayerDead())
            {
                accumulator_ = stepInterval_ * 0.999f; // congela la scena sull'ultimo passo
                break;
            }
        }
    }

    view_.onModelEvents(model_.getEvents(), model_);
    model_.clearEvents();
}
