# Snake MVC (C++ & raylib)

Gioco di Snake in C++17 con raylib, strutturato con architettura MVC e compilato con CMake.

## Struttura

```text
snake_mvc/
├── CMakeLists.txt
├── .gitignore
├── README.md
├── include/
│   ├── SnakeModel.hpp
│   ├── SnakeView.hpp
│   └── SnakeController.hpp
└── src/
    ├── SnakeModel.cpp
    ├── SnakeView.cpp
    ├── SnakeController.cpp
    └── main.cpp
```

* **Model**: stato del gioco, movimento, cibo, punteggio e collisioni (senza raylib).
* **View**: disegno con raylib (griglia, serpente, cibo, punteggio, Game Over).
* **Controller**: input da tastiera, timer a step e loop principale.

## Comandi

### Compilare ed eseguire (macOS / Linux)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
./build/snake_mvc
```

### Compilare ed eseguire (Windows)

```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\snake_mvc.exe
```

## Controlli

* **Frecce** o **WASD**: muovi il serpente
* **R**: riavvia la partita
* **ESC**: esci


