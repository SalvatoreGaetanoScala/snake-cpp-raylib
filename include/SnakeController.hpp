#pragma once

#include "AppState.hpp"
#include "SnakeModel.hpp"
#include "SnakeView.hpp"

// Gestisce input, macchina a stati dei menu e tempo di gioco.
class SnakeController
{
public:
    SnakeController(SnakeModel &model, SnakeView &view, float stepIntervalSeconds);

    void run();

private:
    void handleGlobalInput();
    void handleScreenInput();
    void update(float dt);

    void setScreen(Screen screen);
    void goToMainMenu();
    void enterColorScreen(Screen screen);
    void startGame();

    // Helper per i tasti
    static bool upPressed();
    static bool downPressed();
    static bool leftPressed();
    static bool rightPressed();
    static bool confirmPressed();
    static bool backPressed();

    SnakeModel &model_;
    SnakeView &view_;
    AppState state_;
    float stepInterval_;
    float accumulator_ = 0.0f;
};
