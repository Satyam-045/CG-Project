cat << 'EOF' > README.md
# Tesla Roadster 2D - Driving & Flight Physics Simulation

A 2D C++ and SFML 3.0 simulation featuring terrain physics, ground-suction mechanics, thruster flight, dynamic obstacle spawning, and a 0-350 km/h digital instrument cluster.

## Tech Stack & Tools
- **Language:** C++
- **Library:** SFML 3.0 (`Graphics`, `Window`, `System`, `Audio`)
- **Compiler & Environment:** `g++` in MSYS2 (UCRT64) on Windows
- **Editor:** Visual Studio Code

## Project Files
- `main.cpp` — Simulation loop, terrain & vehicle physics, audio manager, HUD, and 0-350 km/h digital cluster
- Audio assets (`background.ogg`, `accel.wav`, `thruster.wav`, `crash.wav`, `jump.wav`) placed in the executable directory

## Build & Run (MSYS2 UCRT64)
```bash
g++ main.cpp -o main.exe -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
./main.exe