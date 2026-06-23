# Silent Hill 2 OpenGL Project

Fast setup guide for building and running the project on Windows.

## Quick Start

Open PowerShell in the project root and run:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

This command builds the game, copies the required assets and DLLs into `build/bin`, and launches:

```text
build/bin/opengl.exe
```

To build without launching:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

If the game was already built, run it from:

```powershell
cd .\build\bin
.\opengl.exe
```

Run the executable from `build/bin`, because the game loads shaders, models, sounds, videos, and skybox textures through relative paths.

## Requirements

- Windows 10 or Windows 11.
- GPU/driver with OpenGL 3.3 support.
- MSYS2 installed at `C:\msys64`.
- MinGW 64-bit `g++.exe`.
- MinGW GLFW and GLEW packages.

Install the required MSYS2 packages from the `MSYS2 MinGW x64` terminal:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

The build script automatically looks for:

```text
C:\msys64\mingw64\bin\g++.exe
```

## Project Structure

```text
.
|-- Resource Files/
|   |-- opengl.cpp                  Main OpenGL entry point
|   |-- systems/                    Gameplay/UI/audio modules
|   |   |-- angela_system.inl
|   |   |-- audio_system.inl
|   |   |-- damage_overlay.inl
|   |   |-- enemy_system.inl
|   |   |-- flashlight_system.inl
|   |   |-- health_system.inl
|   |   |-- model_animation_system.inl
|   |   |-- navigation_system.inl
|   |   |-- objectives_hud.inl
|   |   |-- objective_overlay.inl
|   |   |-- pause_menu.inl
|   |   |-- shooting_system.inl
|   |   |-- status_hud.inl
|   |   `-- texture_system.inl
|   |-- *.vs / *.fs                 GLSL shaders
|   |-- shwallpaper.jpeg            Menu wallpaper
|   `-- video_sh.wmv                Intro cinematic
|-- models/
|   |-- town_visual.glb             Main map
|   |-- james/                      James model, sounds, weapons
|   |-- jamesanimations/            James animation clips
|   |-- angela/                     Angela model, animations, sounds, video
|   `-- enemies/                    Pyramid Head and other enemy assets
|-- sounds/                         Music and shared sound effects
|-- skybox/                         Skybox textures
|-- glad/                           OpenGL loader
|-- glm/                            Math library
|-- SOIL2/                          Image loading helpers
|-- assimp/                         Assimp headers/build outputs
|-- .vscode/                        Optional VS Code tasks
|-- build.ps1                       Recommended Windows build script
|-- build.sh                        Shell build helper
|-- CMakeLists.txt                  CMake project file
|-- package.ps1                     Packaging helper
`-- README.md
```

## What `build.ps1` Does

The script:

1. Finds `g++.exe`.
2. Compiles `glad/src/glad.c`.
3. Compiles `Resource Files/opengl.cpp`.
4. Links `build/bin/opengl.exe`.
5. Copies runtime DLLs and assets into `build/bin`.

Copied runtime content includes:

```text
build/bin/Resource Files/
build/bin/models/
build/bin/sounds/
build/bin/skybox/
build/bin/glfw3.dll
build/bin/glew32.dll
build/bin/libassimp-6.dll
build/bin/libstdc++-6.dll
build/bin/libgcc_s_seh-1.dll
build/bin/libwinpthread-1.dll
```

## VS Code

Open the full project folder in VS Code.

Build:

```text
Ctrl+Shift+B
```

Run:

```text
Terminal > Run Task... > Run OpenGL
```

## Map Export Notes

The main map is:

```text
models/town_visual.glb
```

The engine reads these authored nodes from the map:

- `spawn_player`: James spawn position and direction.
- `angelainitialpos`: Angela's starting position.
- `shotgunpos`: shotgun pickup position.
- `savepoint*`: save point markers.
- `walkarea*`: navigation surfaces.
- `lightpos*`: horror light positions.

Keep those names when exporting the GLB again.

## Troubleshooting

### PowerShell blocks the script

Use the full bypass command:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

### `g++.exe` was not found

Install MSYS2 and confirm this file exists:

```text
C:\msys64\mingw64\bin\g++.exe
```

### Missing `-lglfw3` or `-lglew32`

Install GLFW and GLEW:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

### The game opens but assets are missing

Rebuild with:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

Then confirm these folders exist:

```text
build/bin/Resource Files
build/bin/models
build/bin/sounds
build/bin/skybox
```

### The game does not start

Run from a terminal to see the error:

```powershell
cd .\build\bin
.\opengl.exe
```
