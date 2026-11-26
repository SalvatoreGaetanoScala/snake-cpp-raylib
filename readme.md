# Snake MVC - Isometric 3D (C++ & raylib)

Snake in C++17 con raylib 5.5, architettura MVC, compilato con CMake.
Visuale 3D **isometrica esatta** (camera ortografica lungo la diagonale 1,1,1), mappa **infinita**
stile agar.io con mele sparse, modalita' Single player e Multiplayer contro la CPU,
menu ispirati a Final Fantasy VII.

## Struttura

```text
snake_mvc/
├── CMakeLists.txt
├── include/
│   ├── Types.hpp            celle, direzioni, colori disponibili
│   ├── AppState.hpp         schermate e selezioni dei menu
│   ├── SnakeModel.hpp
│   ├── SnakeView.hpp
│   ├── SnakeController.hpp
│   └── Ff7Ui.hpp            finestre blu arrotondate, manina-cursore, testo con ombra
└── src/
    ├── SnakeModel.cpp
    ├── SnakeView.cpp
    ├── SnakeController.cpp
    ├── Ff7Ui.cpp
    └── main.cpp
```

* **Model**: mondo infinito, mele, serpenti, collisioni, IA della CPU (senza raylib).
* **View**: mondo 3D isometrico, HUD con minimappa, menu FF7, schermo intero.
* **Controller**: input, macchina a stati dei menu, pausa, timer a passi.

## Compilare ed eseguire

### macOS / Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
./build/snake_mvc
```

### Windows

```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\snake_mvc.exe
```

## Menu

1. **Avvio / Esci**
2. Avvio apre (in cascata) **Single player / Multiplayer CPU**
3. Scelta del colore del serpente: Verde, Blu, Fucsia, Rosa, Giallo
4. In Multiplayer CPU si sceglie anche il colore della CPU

## Controlli

| Tasto | Azione |
|---|---|
| Frecce / WASD | muovi il serpente (in isometria: Su = alto-destra, Destra = basso-destra, Giu' = basso-sinistra, Sinistra = alto-sinistra) |
| Invio / Spazio | conferma nei menu |
| ESC | pausa (Riprendi / Esci) - nei menu: indietro |
| F11 oppure Alt+Invio | schermo intero (Windows e macOS) |
| Ctrl+Cmd+F | schermo intero (macOS) |

## Regole

* La mappa non ha bordi: si muore solo toccando un corpo (il proprio o quello dell'avversario).
* Ogni mela rossa vale 10 punti e allunga il serpente; piu' sei lungo, piu' la visuale si allarga.
* Quando la CPU viene eliminata il suo corpo diventa mele e lei rientra in gioco dopo qualche secondo.

## Font

Il gioco usa Arial Bold di sistema (Windows/macOS). Per usare un font diverso, ad esempio uno in
stile FF7, metti un file `assets/font.ttf` accanto all'eseguibile.
