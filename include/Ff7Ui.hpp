#pragma once

#include "raylib.h"

// Piccola libreria di widget 2D ispirati alle finestre di Final Fantasy VII:
// riquadro blu sfumato con bordi arrotondati, cornice chiara, testo con ombra
// e la classica "manina" come cursore.
namespace ff7
{
void setFont(Font font, bool custom);

Color fade(Color c, float alpha);
Color shade(Color c, float k);

// Rettangolo arrotondato riempito con sfumatura sui 4 angoli
void fillRoundedGradient(Rectangle r, float radius, Color tl, Color tr, Color bl, Color br);

// Finestra FF7 completa (ombra + cornice + sfondo blu sfumato)
void drawWindow(Rectangle r, float scale, float alpha = 1.0f);

// Testo con ombra
Vector2 measureText(const char *text, float size);
void drawText(const char *text, float x, float y, float size, Color color);
void drawTextCentered(const char *text, float centerX, float y, float size, Color color);

// Cursore a forma di mano; tip = punta dell'indice (la mano sta a sinistra)
void drawHand(Vector2 tip, float scale, float alpha = 1.0f);

// Cubetto isometrico 2D (icone e campioni di colore)
void drawIsoCube(Vector2 center, float size, Color color, float alpha = 1.0f);

// Linea separatrice chiara dentro una finestra
void drawSeparator(float x1, float x2, float y, float scale);

constexpr Color kTextColor{236, 236, 240, 255};
constexpr Color kTextDim{140, 146, 176, 255};
constexpr Color kTextTitle{255, 236, 150, 255};
} // namespace ff7
