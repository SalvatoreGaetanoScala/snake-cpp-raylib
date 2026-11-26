#pragma once

#include "AppState.hpp"
#include "SnakeModel.hpp"

#include "raylib.h"

#include <string>
#include <vector>

// Disegna il mondo 3D isometrico, l'HUD e i menu in stile FF7.
class SnakeView
{
public:
    SnakeView(int width, int height, const char *title);
    ~SnakeView();

    SnakeView(const SnakeView &) = delete;
    SnakeView &operator=(const SnakeView &) = delete;

    bool shouldClose() const;
    void toggleFullscreen();

    void onModelEvents(const std::vector<GameEvent> &events, const SnakeModel &model);
    void resetEffects(const SnakeModel &model); // centra la camera e azzera particelle/messaggi

    void render(const AppState &state, const SnakeModel &model, float alpha);

private:
    struct Particle
    {
        Vector3 pos;
        Vector3 vel;
        Color color;
        float life;
        float maxLife;
        float size;
    };

    // Mondo 3D
    void updateCamera(const SnakeModel &model, float alpha, float dt);
    void updateParticles(float dt);
    void drawWorld(const AppState &state, const SnakeModel &model, float alpha);
    void drawGround();
    void drawSnake(const Snake &snake, float alpha, bool blink);
    void drawApple(Cell c);
    void drawParticles();
    bool inView(float x, float z, float margin) const;
    void spawnBurst(Vector3 p, Color c, int count, float speed);

    // Interfaccia 2D
    float uiScale() const;
    Rectangle virt(float x, float y, float w, float h) const; // coordinate virtuali 1280x720 centrate
    void drawVignette(float strength);
    void drawTitle();
    void drawMainMenu(const AppState &state);
    void drawColorMenu(const AppState &state);
    void drawHud(const AppState &state, const SnakeModel &model);
    void drawMinimap(const SnakeModel &model, Rectangle r);
    void drawPauseMenu(const AppState &state);
    void drawGameOver(const AppState &state, const SnakeModel &model);
    void drawFooter(const char *text);
    void drawMenuItems(Rectangle win, const char *const *items, int count, int selected,
                       float firstY, float stepY, float textSize, bool active);

    Camera3D camera_{};
    Vector3 camTarget_{0.0f, 0.0f, 0.0f};
    float camZoom_ = 22.0f;

    Font font_{};
    bool customFont_ = false;

    std::vector<Particle> particles_;
    std::string toast_;
    float toastTimer_ = 0.0f;
    float time_ = 0.0f;

    int windowedW_ = 1280;
    int windowedH_ = 720;
};
