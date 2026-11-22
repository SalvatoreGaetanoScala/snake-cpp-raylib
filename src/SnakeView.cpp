#include "SnakeView.hpp"

#include "raylib.h"
#include <cstdio>

SnakeView::SnakeView(int cellSize, int hudHeight, const char *title)
    : cellSize_(cellSize), hudHeight_(hudHeight)
{
    // Le dimensioni finestra verranno aggiornate o gestite dal controller/costante
    // Qui usiamo una dimensione iniziale coerente con la griglia predefinita
    InitWindow(25 * cellSize_, hudHeight_ + 25 * cellSize_, title);
    SetTargetFPS(60);
}

SnakeView::~SnakeView()
{
    CloseWindow();
}

bool SnakeView::shouldClose() const
{
    return WindowShouldClose();
}

void SnakeView::render(const SnakeModel &model) const
{
    const int gridW = model.getGridWidth();
    const int gridH = model.getGridHeight();
    const int boardWidth = gridW * cellSize_;
    const int boardHeight = gridH * cellSize_;

    BeginDrawing();
    ClearBackground(Color{24, 24, 28, 255});

    // Barra punteggio in alto
    DrawRectangle(0, 0, boardWidth, hudHeight_, Color{35, 35, 42, 255});
    char scoreText[64];
    std::snprintf(scoreText, sizeof(scoreText), "Punteggio: %d", model.getScore());
    DrawText(scoreText, 16, (hudHeight_ - 24) / 2, 24, RAYWHITE);
    DrawText("Frecce / WASD  |  R: Riavvia", boardWidth - 270, (hudHeight_ - 18) / 2, 18, LIGHTGRAY);

    // Sfondo e griglia dell'area di gioco
    DrawRectangle(0, hudHeight_, boardWidth, boardHeight, Color{18, 18, 22, 255});
    for (int x = 0; x <= gridW; ++x)
    {
        int px = x * cellSize_;
        DrawLine(px, hudHeight_, px, hudHeight_ + boardHeight, Color{30, 30, 36, 255});
    }
    for (int y = 0; y <= gridH; ++y)
    {
        int py = hudHeight_ + y * cellSize_;
        DrawLine(0, py, boardWidth, py, Color{30, 30, 36, 255});
    }

    // Cibo
    const Cell food = model.getFood();
    DrawRectangle(
        food.x * cellSize_ + 2,
        hudHeight_ + food.y * cellSize_ + 2,
        cellSize_ - 4,
        cellSize_ - 4,
        Color{235, 64, 52, 255});

    // Serpente
    const auto &snake = model.getSnake();
    bool isHead = true;
    for (const auto &seg : snake)
    {
        Color color = isHead ? Color{92, 224, 118, 255} : Color{56, 168, 82, 255};
        DrawRectangle(
            seg.x * cellSize_ + 1,
            hudHeight_ + seg.y * cellSize_ + 1,
            cellSize_ - 2,
            cellSize_ - 2,
            color);
        isHead = false;
    }

    // Overlay Game Over
    if (model.isGameOver())
    {
        DrawRectangle(0, hudHeight_, boardWidth, boardHeight, Color{0, 0, 0, 160});
        const char *msg = "GAME OVER";
        const char *sub = "Premi R per ricominciare";
        int msgW = MeasureText(msg, 42);
        int subW = MeasureText(sub, 20);
        int centerY = hudHeight_ + boardHeight / 2;

        DrawText(msg, (boardWidth - msgW) / 2, centerY - 36, 42, Color{255, 90, 90, 255});
        DrawText(sub, (boardWidth - subW) / 2, centerY + 16, 20, RAYWHITE);
    }

    EndDrawing();
}
