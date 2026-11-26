#include "Ff7Ui.hpp"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ff7
{
namespace
{
Font g_font{};
bool g_hasFont = false;
bool g_customFont = false;

constexpr float kPi = 3.14159265358979f;

Font currentFont() { return g_hasFont ? g_font : GetFontDefault(); }

float spacingFor(float size) { return g_customFont ? size * 0.02f : size * 0.1f; }

unsigned char toByte(float v) { return static_cast<unsigned char>(std::clamp(v, 0.0f, 255.0f)); }

Color bilerp(Color tl, Color tr, Color bl, Color br, float u, float v)
{
    auto mixc = [](float a, float b, float t) { return a + (b - a) * t; };
    Color out;
    out.r = toByte(mixc(mixc(tl.r, tr.r, u), mixc(bl.r, br.r, u), v));
    out.g = toByte(mixc(mixc(tl.g, tr.g, u), mixc(bl.g, br.g, u), v));
    out.b = toByte(mixc(mixc(tl.b, tr.b, u), mixc(bl.b, br.b, u), v));
    out.a = toByte(mixc(mixc(tl.a, tr.a, u), mixc(bl.a, br.a, u), v));
    return out;
}

Rectangle inset(Rectangle r, float d) { return {r.x + d, r.y + d, r.width - 2 * d, r.height - 2 * d}; }

void roundedBox(Rectangle r, float roundnessPx, Color c)
{
    float minSide = std::min(r.width, r.height);
    float roundness = minSide > 0 ? std::clamp(roundnessPx * 2.0f / minSide, 0.0f, 1.0f) : 0.0f;
    DrawRectangleRounded(r, roundness, 6, c);
}
} // namespace

void setFont(Font font, bool custom)
{
    g_font = font;
    g_hasFont = true;
    g_customFont = custom;
}

Color fade(Color c, float alpha)
{
    c.a = toByte(c.a * std::clamp(alpha, 0.0f, 1.0f));
    return c;
}

Color shade(Color c, float k)
{
    return Color{toByte(c.r * k), toByte(c.g * k), toByte(c.b * k), c.a};
}

void fillRoundedGradient(Rectangle r, float radius, Color tl, Color tr, Color bl, Color br)
{
    if (r.width <= 0 || r.height <= 0)
    {
        return;
    }
    radius = std::clamp(radius, 0.0f, std::min(r.width, r.height) * 0.5f);
    const int seg = 8;

    std::vector<Vector2> pts;
    pts.reserve(4 * (seg + 1));
    auto arc = [&](float cx, float cy, float a0)
    {
        for (int i = 0; i <= seg; ++i)
        {
            float a = (a0 + 90.0f * static_cast<float>(i) / seg) * kPi / 180.0f;
            pts.push_back({cx + std::cos(a) * radius, cy + std::sin(a) * radius});
        }
    };
    // Angoli in senso orario (y verso il basso): alto-dx, basso-dx, basso-sx, alto-sx
    arc(r.x + r.width - radius, r.y + radius, -90.0f);
    arc(r.x + r.width - radius, r.y + r.height - radius, 0.0f);
    arc(r.x + radius, r.y + r.height - radius, 90.0f);
    arc(r.x + radius, r.y + radius, 180.0f);

    auto colorAt = [&](Vector2 p)
    {
        return bilerp(tl, tr, bl, br, (p.x - r.x) / r.width, (p.y - r.y) / r.height);
    };

    const Vector2 center{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    const Color cc = colorAt(center);
    const std::size_t n = pts.size();

    rlCheckRenderBatchLimit(static_cast<int>(n * 3));
    rlBegin(RL_TRIANGLES);
    for (std::size_t i = 0; i < n; ++i)
    {
        const Vector2 a = pts[i];
        const Vector2 b = pts[(i + 1) % n];
        const Color ca = colorAt(a);
        const Color cb = colorAt(b);
        rlColor4ub(cc.r, cc.g, cc.b, cc.a);
        rlVertex2f(center.x, center.y);
        rlColor4ub(cb.r, cb.g, cb.b, cb.a);
        rlVertex2f(b.x, b.y);
        rlColor4ub(ca.r, ca.g, ca.b, ca.a);
        rlVertex2f(a.x, a.y);
    }
    rlEnd();
}

void drawWindow(Rectangle r, float scale, float alpha)
{
    const float radius = 10.0f * scale;
    const float border = 3.0f * scale;

    // Ombra
    Rectangle sh{r.x + 5 * scale, r.y + 6 * scale, r.width, r.height};
    Color shc = fade(Color{0, 0, 0, 110}, alpha);
    fillRoundedGradient(sh, radius, shc, shc, shc, shc);

    // Cornice chiara (leggermente sfumata, come il bordo "metallico" di FF7)
    fillRoundedGradient(r, radius,
                        fade(Color{240, 240, 250, 255}, alpha), fade(Color{214, 216, 232, 255}, alpha),
                        fade(Color{170, 172, 196, 255}, alpha), fade(Color{132, 134, 162, 255}, alpha));

    // Filo scuro tra cornice e sfondo
    Rectangle mid = inset(r, border * 0.7f);
    Color line = fade(Color{30, 32, 64, 255}, alpha);
    fillRoundedGradient(mid, radius - border * 0.5f, line, line, line, line);

    // Sfondo blu FF7: piu' chiaro in alto a sinistra, blu notte in basso a destra
    Rectangle in = inset(r, border);
    fillRoundedGradient(in, radius - border * 0.7f,
                        fade(Color{46, 92, 200, 255}, alpha), fade(Color{24, 48, 156, 255}, alpha),
                        fade(Color{16, 32, 122, 255}, alpha), fade(Color{6, 10, 64, 255}, alpha));
}

Vector2 measureText(const char *text, float size)
{
    return MeasureTextEx(currentFont(), text, size, spacingFor(size));
}

void drawText(const char *text, float x, float y, float size, Color color)
{
    const float off = std::max(1.0f, size * 0.07f);
    const Font f = currentFont();
    DrawTextEx(f, text, {x + off, y + off}, size, spacingFor(size), Color{0, 0, 0, toByte(color.a * 0.8f)});
    DrawTextEx(f, text, {x, y}, size, spacingFor(size), color);
}

void drawTextCentered(const char *text, float centerX, float y, float size, Color color)
{
    const Vector2 m = measureText(text, size);
    drawText(text, centerX - m.x * 0.5f, y, size, color);
}

void drawHand(Vector2 tip, float scale, float alpha)
{
    const float u = scale * 0.85f;
    const Color outline = fade(Color{24, 24, 36, 255}, alpha);
    const Color glove = fade(Color{250, 250, 252, 255}, alpha);
    const Color glove2 = fade(Color{196, 198, 214, 255}, alpha);
    const Color cuff = fade(Color{170, 176, 200, 255}, alpha);

    struct Part
    {
        Rectangle r;
        Color c;
    };
    const Part parts[] = {
        {{tip.x - 48 * u, tip.y - 7 * u, 10 * u, 16 * u}, cuff},   // polsino
        {{tip.x - 40 * u, tip.y - 8 * u, 21 * u, 18 * u}, glove},  // palmo
        {{tip.x - 27 * u, tip.y - 6 * u, 27 * u, 7 * u}, glove},   // indice
        {{tip.x - 28 * u, tip.y + 1 * u, 13 * u, 5 * u}, glove2},  // dita piegate
        {{tip.x - 29 * u, tip.y + 5 * u, 11 * u, 5 * u}, glove2},
    };

    const float o = 1.6f * u;
    for (const Part &p : parts)
    {
        roundedBox({p.r.x - o, p.r.y - o, p.r.width + 2 * o, p.r.height + 2 * o}, 4 * u, outline);
    }
    for (const Part &p : parts)
    {
        roundedBox(p.r, 3 * u, p.c);
    }
}

void drawIsoCube(Vector2 c, float size, Color color, float alpha)
{
    const float h = size;
    const float w = size * 0.866f;
    const Vector2 top{c.x, c.y - h};
    const Vector2 right{c.x + w, c.y - h * 0.5f};
    const Vector2 left{c.x - w, c.y - h * 0.5f};
    const Vector2 mid{c.x, c.y};
    const Vector2 bottom{c.x, c.y + h};
    const Vector2 rightB{c.x + w, c.y + h * 0.5f};
    const Vector2 leftB{c.x - w, c.y + h * 0.5f};

    const Color topC = fade(shade(color, 1.05f), alpha);
    const Color leftC = fade(shade(color, 0.78f), alpha);
    const Color rightC = fade(shade(color, 0.52f), alpha);

    // L'ordine dei vertici non conta: il culling e' disattivato dalla View
    DrawTriangle(top, left, mid, topC);
    DrawTriangle(top, mid, right, topC);
    DrawTriangle(left, leftB, bottom, leftC);
    DrawTriangle(left, bottom, mid, leftC);
    DrawTriangle(mid, bottom, rightB, rightC);
    DrawTriangle(mid, rightB, right, rightC);

    const Color edge = fade(Color{255, 255, 255, 70}, alpha);
    DrawLineEx(left, mid, 1.0f, edge);
    DrawLineEx(mid, right, 1.0f, edge);
    DrawLineEx(mid, bottom, 1.0f, edge);
}

void drawSeparator(float x1, float x2, float y, float scale)
{
    DrawRectangleRec({x1, y, x2 - x1, 2.0f * scale}, Color{8, 12, 50, 200});
    DrawRectangleRec({x1, y + 2.0f * scale, x2 - x1, 1.0f * scale}, Color{150, 170, 230, 160});
}
} // namespace ff7
