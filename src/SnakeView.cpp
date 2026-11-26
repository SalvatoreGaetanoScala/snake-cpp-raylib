#include "SnakeView.hpp"

#include "Ff7Ui.hpp"

#include "raymath.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace
{
// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
const Color kSkinColors[kSkinCount] = {
    {86, 214, 104, 255},  // Verde
    {64, 140, 245, 255},  // Blu
    {232, 48, 206, 255},  // Fucsia
    {255, 156, 198, 255}, // Rosa
    {250, 212, 56, 255},  // Giallo
};

Color skinColor(int skin) { return kSkinColors[((skin % kSkinCount) + kSkinCount) % kSkinCount]; }

const Color kGroundA{40, 48, 72, 255};
const Color kGroundB{47, 56, 83, 255};
const Color kAppleRed{222, 34, 44, 255};

// Ombreggiatura delle facce (luce dall'alto a sinistra dello schermo).
// Con la camera isometrica in (+1,+1,+1) sono visibili solo: sopra, +Z (lato sinistro), +X (lato destro)
constexpr float kShadeTop = 1.0f;
constexpr float kShadeLeft = 0.76f;  // faccia +Z
constexpr float kShadeRight = 0.54f; // faccia +X

constexpr float kSqrt2 = 1.41421356f;
constexpr float kSqrt6 = 2.44948974f;
constexpr float kPi = 3.14159265f;

std::uint32_t hashCell(int x, int z)
{
    std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(z) * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

void vertexColor(Color c) { rlColor4ub(c.r, c.g, c.b, c.a); }

// Parallelepipedo con facce ombreggiate (disegna solo le 3 facce visibili in isometria)
void drawBox(Vector3 c, Vector3 size, Color col)
{
    const float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
    const float x0 = c.x - hx, x1 = c.x + hx;
    const float y0 = c.y - hy, y1 = c.y + hy;
    const float z0 = c.z - hz, z1 = c.z + hz;

    rlCheckRenderBatchLimit(12);
    rlBegin(RL_QUADS);
    // Sopra
    vertexColor(ff7::shade(col, kShadeTop));
    rlVertex3f(x0, y1, z0);
    rlVertex3f(x0, y1, z1);
    rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y1, z0);
    // +Z
    vertexColor(ff7::shade(col, kShadeLeft));
    rlVertex3f(x0, y0, z1);
    rlVertex3f(x1, y0, z1);
    rlVertex3f(x1, y1, z1);
    rlVertex3f(x0, y1, z1);
    // +X
    vertexColor(ff7::shade(col, kShadeRight));
    rlVertex3f(x1, y0, z0);
    rlVertex3f(x1, y1, z0);
    rlVertex3f(x1, y1, z1);
    rlVertex3f(x1, y0, z1);
    rlEnd();
}

// Ombra morbida a terra
void drawShadow(float x, float z, float radius, unsigned char alpha = 70)
{
    const int seg = 14;
    rlCheckRenderBatchLimit(seg * 3);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < seg; ++i)
    {
        const float a0 = 2.0f * kPi * static_cast<float>(i) / seg;
        const float a1 = 2.0f * kPi * static_cast<float>(i + 1) / seg;
        rlColor4ub(0, 0, 0, alpha);
        rlVertex3f(x, 0.012f, z);
        rlColor4ub(0, 0, 0, 0);
        rlVertex3f(x + std::cos(a0) * radius, 0.012f, z + std::sin(a0) * radius);
        rlVertex3f(x + std::cos(a1) * radius, 0.012f, z + std::sin(a1) * radius);
    }
    rlEnd();
}

// Sfera illuminata (luce calcolata sui vertici: niente shader necessari)
void drawLitSphere(Vector3 center, float radius, Color base)
{
    constexpr int kRings = 10;
    constexpr int kSlices = 16;
    struct V
    {
        Vector3 n;
        float light;
        float spec;
    };
    static V grid[kRings + 1][kSlices + 1];
    static bool ready = false;
    if (!ready)
    {
        const Vector3 L = Vector3Normalize({-0.3f, 1.0f, 0.5f});
        const Vector3 viewDir = Vector3Normalize({1.0f, 1.0f, 1.0f});
        const Vector3 H = Vector3Normalize(Vector3Add(L, viewDir));
        for (int r = 0; r <= kRings; ++r)
        {
            const float theta = kPi * static_cast<float>(r) / kRings;
            for (int s = 0; s <= kSlices; ++s)
            {
                const float phi = 2.0f * kPi * static_cast<float>(s) / kSlices;
                Vector3 n{std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)};
                grid[r][s].n = n;
                grid[r][s].light = 0.42f + 0.58f * std::max(0.0f, Vector3DotProduct(n, L));
                grid[r][s].spec = std::pow(std::max(0.0f, Vector3DotProduct(n, H)), 24.0f);
            }
        }
        ready = true;
    }

    auto emit = [&](const V &v)
    {
        const float sp = v.spec * 200.0f;
        rlColor4ub(static_cast<unsigned char>(std::min(255.0f, base.r * v.light + sp)),
                   static_cast<unsigned char>(std::min(255.0f, base.g * v.light + sp)),
                   static_cast<unsigned char>(std::min(255.0f, base.b * v.light + sp)),
                   base.a);
        rlVertex3f(center.x + v.n.x * radius, center.y + v.n.y * radius, center.z + v.n.z * radius);
    };

    rlCheckRenderBatchLimit(kRings * kSlices * 6);
    rlBegin(RL_TRIANGLES);
    for (int r = 0; r < kRings; ++r)
    {
        for (int s = 0; s < kSlices; ++s)
        {
            emit(grid[r][s]);
            emit(grid[r + 1][s]);
            emit(grid[r + 1][s + 1]);
            emit(grid[r][s]);
            emit(grid[r + 1][s + 1]);
            emit(grid[r][s + 1]);
        }
    }
    rlEnd();
}

// Posizione interpolata (fluida) del segmento i fra il passo precedente e quello attuale
Vector3 segmentPos(const Snake &s, std::size_t i, float alpha)
{
    const Cell cur = s.body[i];
    Cell prev = cur;
    if (!s.prevBody.empty())
    {
        prev = s.prevBody[std::min(i, s.prevBody.size() - 1)];
    }
    if (manhattan(prev, cur) > 1)
    {
        prev = cur;
    }
    return {Lerp(static_cast<float>(prev.x), static_cast<float>(cur.x), alpha), 0.0f,
            Lerp(static_cast<float>(prev.y), static_cast<float>(cur.y), alpha)};
}

Vector3 dirVector(Direction d)
{
    switch (d)
    {
    case Direction::Up: return {0, 0, -1};
    case Direction::Down: return {0, 0, 1};
    case Direction::Left: return {-1, 0, 0};
    case Direction::Right: return {1, 0, 0};
    }
    return {1, 0, 0};
}

std::string findFontPath()
{
    std::vector<std::string> candidates = {
        std::string(GetApplicationDirectory()) + "assets/font.ttf", // font personalizzato opzionale
        "assets/font.ttf",
#if defined(_WIN32)
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arial.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
#endif
    };
    for (const std::string &p : candidates)
    {
        if (FileExists(p.c_str()))
        {
            return p;
        }
    }
    return {};
}
} // namespace

// ===========================================================================
// Finestra
// ===========================================================================

SnakeView::SnakeView(int width, int height, const char *title)
    : windowedW_(width), windowedH_(height)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    SetWindowMinSize(800, 450);
    SetExitKey(KEY_NULL); // ESC serve per la pausa, non per chiudere
    SetTargetFPS(60);

    const std::string fontPath = findFontPath();
    if (!fontPath.empty())
    {
        font_ = LoadFontEx(fontPath.c_str(), 64, nullptr, 0);
        if (font_.texture.id != 0 && font_.glyphCount > 0)
        {
            GenTextureMipmaps(&font_.texture);
            SetTextureFilter(font_.texture, TEXTURE_FILTER_TRILINEAR);
            customFont_ = true;
        }
    }
    if (!customFont_)
    {
        font_ = GetFontDefault();
    }
    ff7::setFont(font_, customFont_);

    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.projection = CAMERA_ORTHOGRAPHIC;
    camera_.fovy = camZoom_;
}

SnakeView::~SnakeView()
{
    if (customFont_)
    {
        UnloadFont(font_);
    }
    CloseWindow();
}

bool SnakeView::shouldClose() const
{
    return WindowShouldClose();
}

void SnakeView::toggleFullscreen()
{
#if defined(__APPLE__)
    // Su macOS lo schermo intero "vero" porta la finestra alla risoluzione del monitor
    if (!IsWindowFullscreen())
    {
        windowedW_ = GetScreenWidth();
        windowedH_ = GetScreenHeight();
        const int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }
    else
    {
        ToggleFullscreen();
        SetWindowSize(windowedW_, windowedH_);
    }
#else
    // Su Windows/Linux: finestra senza bordi grande quanto il monitor (veloce e senza cambi di risoluzione)
    ToggleBorderlessWindowed();
#endif
}

// ===========================================================================
// Eventi / camera / particelle
// ===========================================================================

void SnakeView::resetEffects(const SnakeModel &model)
{
    particles_.clear();
    toast_.clear();
    toastTimer_ = 0.0f;
    const auto &snakes = model.getSnakes();
    if (!snakes.empty() && !snakes[0].body.empty())
    {
        const Cell h = snakes[0].head();
        camTarget_ = {static_cast<float>(h.x), 0.0f, static_cast<float>(h.y)};
    }
}

void SnakeView::spawnBurst(Vector3 p, Color c, int count, float speed)
{
    for (int i = 0; i < count; ++i)
    {
        const float a = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
        const float sp = speed * (0.4f + static_cast<float>(GetRandomValue(0, 100)) / 160.0f);
        Particle part;
        part.pos = p;
        part.vel = {std::cos(a) * sp, 2.5f + static_cast<float>(GetRandomValue(0, 100)) / 30.0f, std::sin(a) * sp};
        part.color = c;
        part.maxLife = 0.5f + static_cast<float>(GetRandomValue(0, 40)) / 100.0f;
        part.life = part.maxLife;
        part.size = 0.12f + static_cast<float>(GetRandomValue(0, 10)) / 100.0f;
        particles_.push_back(part);
    }
}

void SnakeView::onModelEvents(const std::vector<GameEvent> &events, const SnakeModel &model)
{
    const auto &snakes = model.getSnakes();
    for (const GameEvent &e : events)
    {
        const Vector3 p{static_cast<float>(e.cell.x), 0.5f, static_cast<float>(e.cell.y)};
        const bool validSnake = e.snakeIndex >= 0 && e.snakeIndex < static_cast<int>(snakes.size());
        const Color sc = validSnake ? skinColor(snakes[e.snakeIndex].skin) : WHITE;
        const bool cpu = validSnake && snakes[e.snakeIndex].isCpu;

        switch (e.type)
        {
        case GameEvent::Type::AppleEaten:
            spawnBurst(p, kAppleRed, 10, 2.5f);
            spawnBurst(p, Color{90, 200, 70, 255}, 3, 2.0f);
            break;
        case GameEvent::Type::SnakeDied:
            spawnBurst(p, sc, 30, 4.0f);
            if (!model.isDemo() && cpu)
            {
                toast_ = "CPU eliminata!";
                toastTimer_ = 2.2f;
            }
            break;
        case GameEvent::Type::SnakeRespawned:
            spawnBurst(p, Color{255, 255, 255, 255}, 16, 2.0f);
            if (!model.isDemo() && cpu)
            {
                toast_ = "La CPU e' tornata in gioco";
                toastTimer_ = 2.2f;
            }
            break;
        }
    }
}

void SnakeView::updateCamera(const SnakeModel &model, float alpha, float dt)
{
    const auto &snakes = model.getSnakes();
    Vector3 target = camTarget_;
    float desiredZoom = camZoom_;
    if (!snakes.empty() && !snakes[0].body.empty())
    {
        target = segmentPos(snakes[0], 0, alpha);
        const float len = static_cast<float>(snakes[0].body.size());
        // Come in agar.io: piu' sei lungo, piu' la visuale si allarga
        desiredZoom = 20.0f + std::min(len, 120.0f) * 0.06f;
    }

    camTarget_ = Vector3Lerp(camTarget_, target, 1.0f - std::exp(-dt * 7.0f));
    camZoom_ = Lerp(camZoom_, desiredZoom, 1.0f - std::exp(-dt * 2.0f));

    // Proiezione ortografica lungo la diagonale (1,1,1): isometria esatta
    // (azimut 45 gradi, elevazione 35.264 gradi, i tre assi accorciati allo stesso modo)
    camera_.target = camTarget_;
    camera_.position = Vector3Add(camTarget_, {30.0f, 30.0f, 30.0f});
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = camZoom_;
    camera_.projection = CAMERA_ORTHOGRAPHIC;
}

void SnakeView::updateParticles(float dt)
{
    for (Particle &p : particles_)
    {
        p.vel.y -= 12.0f * dt;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        if (p.pos.y < 0.06f)
        {
            p.pos.y = 0.06f;
            p.vel.y *= -0.35f;
            p.vel.x *= 0.6f;
            p.vel.z *= 0.6f;
        }
        p.life -= dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle &p) { return p.life <= 0.0f; }),
                     particles_.end());
}

// ===========================================================================
// Mondo 3D
// ===========================================================================

bool SnakeView::inView(float x, float z, float margin) const
{
    const float aspect = static_cast<float>(GetScreenWidth()) / std::max(1, GetScreenHeight());
    const float hh = camZoom_ * 0.5f;
    const float hw = hh * aspect;
    const float dx = x - camTarget_.x;
    const float dz = z - camTarget_.z;
    const float sx = (dx - dz) / kSqrt2;
    const float sy = (dx + dz) / kSqrt6;
    return std::fabs(sx) <= hw + margin && std::fabs(sy) <= hh + margin;
}

void SnakeView::drawGround()
{
    const float aspect = static_cast<float>(GetScreenWidth()) / std::max(1, GetScreenHeight());
    const float hh = camZoom_ * 0.5f;
    const float hw = hh * aspect;
    const int range = static_cast<int>(std::ceil((hw * kSqrt2 + hh * kSqrt6) * 0.5f)) + 2;
    const int cx = static_cast<int>(std::round(camTarget_.x));
    const int cz = static_cast<int>(std::round(camTarget_.z));

    // Pavimento a scacchi infinito (si disegnano solo le celle visibili)
    for (int z = cz - range; z <= cz + range; ++z)
    {
        for (int x = cx - range; x <= cx + range; ++x)
        {
            const float fx = static_cast<float>(x);
            const float fz = static_cast<float>(z);
            if (!inView(fx, fz, 1.5f))
            {
                continue;
            }
            Color c = ((x + z) & 1) ? kGroundA : kGroundB;
            const std::uint32_t h = hashCell(x, z);
            const float k = 0.96f + static_cast<float>(h % 9) * 0.01f;
            c = ff7::shade(c, k);

            rlCheckRenderBatchLimit(4);
            rlBegin(RL_QUADS);
            vertexColor(c);
            rlVertex3f(fx - 0.5f, 0.0f, fz - 0.5f);
            rlVertex3f(fx - 0.5f, 0.0f, fz + 0.5f);
            rlVertex3f(fx + 0.5f, 0.0f, fz + 0.5f);
            rlVertex3f(fx + 0.5f, 0.0f, fz - 0.5f);
            rlEnd();
        }
    }

    // Sassolini sparsi: danno il senso del movimento nella mappa infinita
    for (int z = cz - range; z <= cz + range; ++z)
    {
        for (int x = cx - range; x <= cx + range; ++x)
        {
            const std::uint32_t h = hashCell(x * 3 + 7, z * 5 - 11);
            if (h % 19 != 0 || !inView(static_cast<float>(x), static_cast<float>(z), 1.0f))
            {
                continue;
            }
            const float ox = static_cast<float>((h >> 8) % 60) / 100.0f - 0.3f;
            const float oz = static_cast<float>((h >> 16) % 60) / 100.0f - 0.3f;
            const float sz = 0.12f + static_cast<float>((h >> 4) % 10) / 100.0f;
            drawBox({x + ox, 0.03f, z + oz}, {sz, 0.06f, sz}, Color{70, 82, 116, 255});
        }
    }
}

void SnakeView::drawApple(Cell c)
{
    const float x = static_cast<float>(c.x);
    const float z = static_cast<float>(c.y);
    const float bob = std::sin(time_ * 3.0f + x * 0.7f + z * 1.3f) * 0.06f;

    drawShadow(x, z, 0.34f, 80);
    drawLitSphere({x, 0.42f + bob, z}, 0.34f, kAppleRed);
    drawBox({x, 0.82f + bob, z}, {0.06f, 0.18f, 0.06f}, Color{112, 72, 32, 255});      // picciolo
    drawBox({x + 0.11f, 0.83f + bob, z}, {0.18f, 0.05f, 0.11f}, Color{80, 196, 64, 255}); // foglia
}

void SnakeView::drawSnake(const Snake &snake, float alpha, bool blink)
{
    if (snake.body.empty())
    {
        return;
    }
    if (blink && std::fmod(time_, 0.24f) < 0.1f)
    {
        return;
    }

    Color base = skinColor(snake.skin);
    if (!snake.alive)
    {
        base = Color{static_cast<unsigned char>(Lerp(base.r, 120.0f, 0.45f)),
                     static_cast<unsigned char>(Lerp(base.g, 120.0f, 0.45f)),
                     static_cast<unsigned char>(Lerp(base.b, 130.0f, 0.45f)), 255};
    }

    const std::size_t n = snake.body.size();
    std::vector<Vector3> pos(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        pos[i] = segmentPos(snake, i, alpha);
    }

    auto taperAt = [n](std::size_t i)
    {
        const float t = n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
        return std::clamp((t - 0.55f) / 0.45f, 0.0f, 1.0f);
    };
    auto widthAt = [&](std::size_t i) { return i == 0 ? 0.94f : 0.84f - 0.26f * taperAt(i); };
    auto heightAt = [&](std::size_t i) { return i == 0 ? 0.80f : 0.66f - 0.20f * taperAt(i); };

    // Ombre
    for (std::size_t i = 0; i < n; ++i)
    {
        drawShadow(pos[i].x, pos[i].z, widthAt(i) * 0.62f);
    }

    // Raccordi fra i segmenti (il corpo sembra continuo anche nelle curve)
    for (std::size_t i = 0; i + 1 < n; ++i)
    {
        const Vector3 mid = Vector3Lerp(pos[i], pos[i + 1], 0.5f);
        const float w = widthAt(i + 1) * 0.82f;
        const float h = heightAt(i + 1) * 0.86f;
        drawBox({mid.x, h * 0.5f + 0.02f, mid.z}, {w, h, w}, ff7::shade(base, 0.82f));
    }

    // Segmenti (dalla coda alla testa)
    for (std::size_t k = n; k-- > 0;)
    {
        const float w = widthAt(k);
        const float h = heightAt(k);
        Color c = base;
        if (k == 0)
        {
            c = ff7::shade(base, 1.1f);
        }
        else if (k % 2 == 1)
        {
            c = ff7::shade(base, 0.9f);
        }
        drawBox({pos[k].x, h * 0.5f + 0.02f, pos[k].z}, {w, h, w}, c);
    }

    // Occhi
    const Vector3 f = dirVector(snake.dir);
    const Vector3 side{-f.z, 0.0f, f.x};
    const float hh = heightAt(0);
    for (int sgn = -1; sgn <= 1; sgn += 2)
    {
        const Vector3 eye{pos[0].x + f.x * 0.38f + side.x * 0.22f * sgn, hh * 0.78f,
                          pos[0].z + f.z * 0.38f + side.z * 0.22f * sgn};
        drawBox(eye, {0.2f, 0.22f, 0.2f}, Color{250, 250, 250, 255});
        const Vector3 pupil{eye.x + f.x * 0.07f, eye.y + 0.01f, eye.z + f.z * 0.07f};
        drawBox(pupil, {0.11f, 0.14f, 0.11f}, Color{20, 20, 32, 255});
    }
}

void SnakeView::drawParticles()
{
    for (const Particle &p : particles_)
    {
        const float k = p.life / p.maxLife;
        const float s = p.size * (0.4f + 0.6f * k);
        drawBox(p.pos, {s, s, s}, p.color);
    }
}

void SnakeView::drawWorld(const AppState &state, const SnakeModel &model, float alpha)
{
    (void)state;
    drawGround();

    for (const Cell &a : model.getApples())
    {
        if (inView(static_cast<float>(a.x), static_cast<float>(a.y), 1.5f))
        {
            drawApple(a);
        }
    }

    for (const Snake &s : model.getSnakes())
    {
        const bool blink = !s.alive && !s.isCpu; // il giocatore eliminato lampeggia
        drawSnake(s, alpha, blink);
    }

    drawParticles();
}

// ===========================================================================
// Interfaccia 2D
// ===========================================================================

float SnakeView::uiScale() const
{
    return std::min(GetScreenWidth() / 1280.0f, GetScreenHeight() / 720.0f);
}

Rectangle SnakeView::virt(float x, float y, float w, float h) const
{
    const float s = uiScale();
    const float ox = (GetScreenWidth() - 1280.0f * s) * 0.5f;
    const float oy = (GetScreenHeight() - 720.0f * s) * 0.5f;
    return {ox + x * s, oy + y * s, w * s, h * s};
}

void SnakeView::drawVignette(float strength)
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    const unsigned char a = static_cast<unsigned char>(std::clamp(200.0f * strength, 0.0f, 255.0f));
    DrawRectangleGradientV(0, 0, w, h / 3, Color{4, 6, 26, a}, Color{4, 6, 26, 0});
    DrawRectangleGradientV(0, h - h / 3, w, h / 3, Color{4, 6, 26, 0}, Color{4, 6, 26, a});
}

void SnakeView::drawMenuItems(Rectangle win, const char *const *items, int count, int selected,
                              float firstY, float stepY, float textSize, bool active)
{
    const float s = uiScale();
    const float x = win.x + 64.0f * s;
    for (int i = 0; i < count; ++i)
    {
        const float y = win.y + (firstY + stepY * static_cast<float>(i)) * s;
        ff7::drawText(items[i], x, y, textSize * s, ff7::kTextColor);
    }
    if (selected >= 0 && selected < count)
    {
        const float y = win.y + (firstY + stepY * static_cast<float>(selected)) * s;
        const float bob = active ? std::sin(time_ * 9.0f) * 2.5f * s : 0.0f;
        ff7::drawHand({x - 8.0f * s + bob, y + textSize * s * 0.5f}, s, active ? 1.0f : 0.55f);
    }
}

void SnakeView::drawFooter(const char *text)
{
    const float s = uiScale();
    const Rectangle r = virt(170, 654, 940, 44);
    ff7::drawWindow(r, s);
    const float size = 19.0f * s;
    ff7::drawTextCentered(text, r.x + r.width * 0.5f, r.y + (r.height - size) * 0.5f, size, ff7::kTextColor);
}

void SnakeView::drawTitle()
{
    const float s = uiScale();
    const Rectangle area = virt(0, 0, 1280, 720);
    const char *word = "SNAKE";
    const float size = 132.0f * s;

    float total = 0.0f;
    float widths[5];
    for (int i = 0; i < 5; ++i)
    {
        const char letter[2] = {word[i], 0};
        widths[i] = ff7::measureText(letter, size).x + 8.0f * s;
        total += widths[i];
    }
    float x = area.x + (area.width - total) * 0.5f;
    for (int i = 0; i < 5; ++i)
    {
        const char letter[2] = {word[i], 0};
        const float y = area.y + 100.0f * s + std::sin(time_ * 2.0f + i * 0.7f) * 5.0f * s;
        // contorno scuro
        for (int ox = -1; ox <= 1; ++ox)
        {
            for (int oy = -1; oy <= 1; ++oy)
            {
                DrawTextEx(font_, letter, {x + ox * 3.0f * s, y + oy * 3.0f * s}, size, 0, Color{8, 10, 40, 255});
            }
        }
        ff7::drawText(letter, x, y, size, skinColor(i));
        x += widths[i];
    }
    ff7::drawTextCentered("ISOMETRIC  3D", area.x + area.width * 0.5f, area.y + 248.0f * s, 30.0f * s,
                          ff7::kTextTitle);
}

void SnakeView::drawMainMenu(const AppState &state)
{
    const float s = uiScale();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{6, 8, 34, 80});
    drawVignette(1.0f);
    drawTitle();

    const bool sub = state.screen == Screen::ModeMenu;

    const Rectangle mainWin = virt(520, 400, 240, 128);
    ff7::drawWindow(mainWin, s);
    static const char *const mainItems[] = {"Avvio", "Esci"};
    drawMenuItems(mainWin, mainItems, 2, state.mainIndex, 24, 48, 32, !sub);

    if (sub)
    {
        // Finestra in cascata che si "apre" come in FF7
        const float t = std::min(1.0f, state.screenTime / 0.14f);
        const Rectangle full = virt(650, 456, 340, 128);
        const Rectangle anim{full.x, full.y + full.height * (1.0f - t) * 0.5f, full.width, full.height * t};
        ff7::drawWindow(anim, s);
        if (t >= 1.0f)
        {
            static const char *const modeItems[] = {"Single player", "Multiplayer CPU"};
            drawMenuItems(full, modeItems, 2, state.modeIndex, 24, 48, 32, true);
        }
    }

    drawFooter("Frecce / WASD: scegli      INVIO: conferma      ESC: indietro      F11: schermo intero");
}

void SnakeView::drawColorMenu(const AppState &state)
{
    const float s = uiScale();
    drawVignette(0.9f);

    const bool cpuStep = state.screen == Screen::ColorCpu;
    const bool versus = state.mode == GameMode::VersusCpu;

    // Titolo
    const Rectangle titleWin = virt(290, 54, 700, 80);
    ff7::drawWindow(titleWin, s);
    const char *title = cpuStep ? "Scegli il colore della CPU" : "Scegli il colore del tuo serpente";
    ff7::drawTextCentered(title, titleWin.x + titleWin.width * 0.5f, titleWin.y + 14.0f * s, 32.0f * s,
                          ff7::kTextColor);
    const char *subtitle = versus ? (cpuStep ? "Multiplayer CPU  -  passo 2 di 2" : "Multiplayer CPU  -  passo 1 di 2")
                                  : "Single player";
    ff7::drawTextCentered(subtitle, titleWin.x + titleWin.width * 0.5f, titleWin.y + 50.0f * s, 18.0f * s,
                          ff7::kTextTitle);

    // Campioni di colore
    const Rectangle win = virt(190, 452, 900, 184);
    ff7::drawWindow(win, s);
    const int selected = cpuStep ? state.cpuSkin : state.playerSkin;
    for (int i = 0; i < kSkinCount; ++i)
    {
        const float cx = win.x + (90.0f + 180.0f * i) * s;
        const float cy = win.y + 76.0f * s;
        const bool disabled = cpuStep && i == state.playerSkin;
        const bool sel = i == selected;
        const float size = (sel ? 34.0f + 3.0f * std::sin(time_ * 5.0f) : 27.0f) * s;

        ff7::drawIsoCube({cx, cy}, size, skinColor(i), disabled ? 0.3f : 1.0f);

        const Color nameColor = sel ? ff7::kTextTitle : (disabled ? ff7::kTextDim : ff7::kTextColor);
        ff7::drawTextCentered(skinName(i), cx, win.y + 124.0f * s, 24.0f * s, nameColor);
        if (disabled)
        {
            ff7::drawTextCentered("(tu)", cx, win.y + 152.0f * s, 18.0f * s, ff7::kTextDim);
        }
        if (sel)
        {
            const float bob = std::sin(time_ * 9.0f) * 2.5f * s;
            ff7::drawHand({cx - size * 0.9f - 10.0f * s + bob, cy}, s);
        }
    }

    drawFooter("Frecce / A D: cambia colore      INVIO: conferma      ESC: indietro");
}

void SnakeView::drawMinimap(const SnakeModel &model, Rectangle r)
{
    const float s = uiScale();
    ff7::drawWindow(r, s);
    const Rectangle in{r.x + 8 * s, r.y + 8 * s, r.width - 16 * s, r.height - 16 * s};

    const auto &snakes = model.getSnakes();
    const float fx = camTarget_.x;
    const float fz = camTarget_.z;
    const float rangeCells = 34.0f;
    const float k = (in.width * 0.5f) / rangeCells;
    const Vector2 c{in.x + in.width * 0.5f, in.y + in.height * 0.5f};

    // Stessa rotazione della visuale isometrica
    auto project = [&](float x, float z)
    {
        const float dx = x - fx;
        const float dz = z - fz;
        return Vector2{c.x + (dx - dz) * 0.7071f * k, c.y + (dx + dz) * 0.4082f * k};
    };

    BeginScissorMode(static_cast<int>(in.x), static_cast<int>(in.y), static_cast<int>(in.width),
                     static_cast<int>(in.height));
    const float dot = std::max(2.0f, 3.0f * s);
    for (const Cell &a : model.getApples())
    {
        const Vector2 p = project(static_cast<float>(a.x), static_cast<float>(a.y));
        DrawRectangleV({p.x - dot * 0.5f, p.y - dot * 0.5f}, {dot, dot}, Color{235, 60, 60, 255});
    }
    for (const Snake &sn : snakes)
    {
        const Color col = skinColor(sn.skin);
        for (std::size_t i = sn.body.size(); i-- > 0;)
        {
            const Vector2 p = project(static_cast<float>(sn.body[i].x), static_cast<float>(sn.body[i].y));
            const float d = i == 0 ? dot * 1.9f : dot * 1.2f;
            if (i == 0)
            {
                DrawRectangleV({p.x - d * 0.5f - 1, p.y - d * 0.5f - 1}, {d + 2, d + 2}, WHITE);
            }
            DrawRectangleV({p.x - d * 0.5f, p.y - d * 0.5f}, {d, d}, col);
        }
    }
    EndScissorMode();
}

void SnakeView::drawHud(const AppState &state, const SnakeModel &model)
{
    (void)state;
    const float s = uiScale();
    const float m = 16.0f * s;
    const auto &snakes = model.getSnakes();
    const int rows = static_cast<int>(snakes.size());

    const Rectangle r{m, m, 360.0f * s, (22.0f + 38.0f * rows) * s};
    ff7::drawWindow(r, s);
    for (int i = 0; i < rows; ++i)
    {
        const Snake &sn = snakes[i];
        const float y = r.y + (14.0f + 38.0f * i) * s;
        ff7::drawIsoCube({r.x + 30.0f * s, y + 13.0f * s}, 11.0f * s, skinColor(sn.skin));
        ff7::drawText(sn.isCpu ? "CPU" : "Tu", r.x + 52.0f * s, y, 24.0f * s, ff7::kTextTitle);

        char buf[64];
        if (sn.alive || !sn.isCpu)
        {
            std::snprintf(buf, sizeof(buf), "%d pt", sn.score);
            ff7::drawText(buf, r.x + 120.0f * s, y, 24.0f * s, ff7::kTextColor);
            std::snprintf(buf, sizeof(buf), "lung. %d", static_cast<int>(sn.body.size()));
            ff7::drawText(buf, r.x + 230.0f * s, y, 24.0f * s, ff7::kTextColor);
        }
        else
        {
            ff7::drawText("in arrivo...", r.x + 120.0f * s, y, 24.0f * s, ff7::kTextDim);
        }
    }

    const float mapSize = 190.0f * s;
    drawMinimap(model, {GetScreenWidth() - m - mapSize, m, mapSize, mapSize});

    const char *hint = "ESC: pausa     F11: schermo intero";
    const float hs = 18.0f * s;
    const Vector2 hm = ff7::measureText(hint, hs);
    ff7::drawText(hint, GetScreenWidth() - m - hm.x, GetScreenHeight() - m - hs, hs, ff7::kTextColor);

    if (toastTimer_ > 0.0f && !toast_.empty())
    {
        const float a = std::min(1.0f, toastTimer_ / 0.4f);
        const float ts = 24.0f * s;
        const float tw = ff7::measureText(toast_.c_str(), ts).x + 56.0f * s;
        const Rectangle tr{(GetScreenWidth() - tw) * 0.5f, m, tw, 50.0f * s};
        ff7::drawWindow(tr, s, a);
        ff7::drawTextCentered(toast_.c_str(), tr.x + tr.width * 0.5f, tr.y + (tr.height - ts) * 0.5f, ts,
                              ff7::fade(ff7::kTextColor, a));
    }
}

void SnakeView::drawPauseMenu(const AppState &state)
{
    const float s = uiScale();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 10, 130});

    const Rectangle w = virt(490, 226, 300, 236);
    ff7::drawWindow(w, s);
    ff7::drawTextCentered("PAUSA", w.x + w.width * 0.5f, w.y + 20.0f * s, 38.0f * s, ff7::kTextTitle);
    ff7::drawSeparator(w.x + 18.0f * s, w.x + w.width - 18.0f * s, w.y + 76.0f * s, s);

    static const char *const items[] = {"Riprendi", "Esci"};
    drawMenuItems(w, items, 2, state.pauseIndex, 102, 52, 32, true);

    drawFooter("Frecce / WASD: scegli      INVIO: conferma      ESC: riprendi");
}

void SnakeView::drawGameOver(const AppState &state, const SnakeModel &model)
{
    const float s = uiScale();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{20, 0, 10, 130});

    const Rectangle w = virt(420, 168, 440, 340);
    ff7::drawWindow(w, s);
    ff7::drawTextCentered("GAME OVER", w.x + w.width * 0.5f, w.y + 22.0f * s, 46.0f * s, Color{255, 120, 110, 255});

    const auto &snakes = model.getSnakes();
    char buf[96];
    if (!snakes.empty())
    {
        std::snprintf(buf, sizeof(buf), "Punteggio: %d    Lunghezza: %d", snakes[0].score,
                      static_cast<int>(snakes[0].body.size()));
        ff7::drawTextCentered(buf, w.x + w.width * 0.5f, w.y + 94.0f * s, 24.0f * s, ff7::kTextColor);
    }
    if (snakes.size() > 1)
    {
        std::snprintf(buf, sizeof(buf), "CPU: %d punti", snakes[1].score);
        ff7::drawTextCentered(buf, w.x + w.width * 0.5f, w.y + 130.0f * s, 22.0f * s, ff7::kTextDim);
    }
    ff7::drawSeparator(w.x + 18.0f * s, w.x + w.width - 18.0f * s, w.y + 176.0f * s, s);

    static const char *const items[] = {"Riprova", "Menu principale"};
    drawMenuItems(w, items, 2, state.gameOverIndex, 204, 52, 32, true);

    drawFooter("Frecce / WASD: scegli      INVIO: conferma");
}

// ===========================================================================
// Render principale
// ===========================================================================

void SnakeView::render(const AppState &state, const SnakeModel &model, float alpha)
{
    const float dt = GetFrameTime();
    time_ += dt;

    updateCamera(model, alpha, dt);
    if (state.screen != Screen::Paused)
    {
        updateParticles(dt);
        if (toastTimer_ > 0.0f)
        {
            toastTimer_ -= dt;
        }
    }

    BeginDrawing();
    rlDisableBackfaceCulling(); // le primitive "fatte a mano" non devono preoccuparsi dell'ordine dei vertici
    ClearBackground(Color{12, 14, 26, 255});

    BeginMode3D(camera_);
    drawWorld(state, model, alpha);
    EndMode3D();

    switch (state.screen)
    {
    case Screen::MainMenu:
    case Screen::ModeMenu:
        drawMainMenu(state);
        break;
    case Screen::ColorPlayer:
    case Screen::ColorCpu:
        drawColorMenu(state);
        break;
    case Screen::Playing:
        drawVignette(0.35f);
        drawHud(state, model);
        break;
    case Screen::Paused:
        drawVignette(0.35f);
        drawHud(state, model);
        drawPauseMenu(state);
        break;
    case Screen::GameOver:
        drawVignette(0.35f);
        drawHud(state, model);
        drawGameOver(state, model);
        break;
    }

    EndDrawing();
}
