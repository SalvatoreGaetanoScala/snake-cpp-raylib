#pragma once

#include "Types.hpp"

// Schermate dell'applicazione
enum class Screen
{
    MainMenu,    // Avvio / Esci
    ModeMenu,    // finestra in cascata: Single player / Multiplayer CPU
    ColorPlayer, // scelta colore del giocatore
    ColorCpu,    // scelta colore della CPU (solo multiplayer)
    Playing,
    Paused,      // Riprendi / Esci
    GameOver     // Riprova / Menu principale
};

// Stato dell'interfaccia: gestito dal Controller, letto dalla View.
struct AppState
{
    Screen screen = Screen::MainMenu;

    int mainIndex = 0;     // 0 = Avvio, 1 = Esci
    int modeIndex = 0;     // 0 = Single player, 1 = Multiplayer CPU
    int pauseIndex = 0;    // 0 = Riprendi, 1 = Esci
    int gameOverIndex = 0; // 0 = Riprova, 1 = Menu principale

    GameMode mode = GameMode::SinglePlayer;
    int playerSkin = 0; // Verde
    int cpuSkin = 2;    // Fucsia

    float screenTime = 0.0f; // secondi dall'apertura della schermata (animazioni)
    float deathTimer = 0.0f;
    bool quit = false;
};
