# Silent Hill 2 OpenGL Project

## English

Fast setup guide for building and running the project on Windows.

### Run The Project

Open PowerShell in the project root and run this command:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

This is the intended way to run the project. It recompiles the code, copies the required assets and DLLs into `build/bin`, and launches the game immediately, so your latest changes are always reflected.

### Requirements

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

### Project Structure

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
|-- build.ps1                       Build-and-run script
|-- build.sh                        Shell build helper
|-- CMakeLists.txt                  CMake project file
|-- package.ps1                     Packaging helper
`-- README.md
```

### What `build.ps1` Does

1. Finds `g++.exe`.
2. Compiles `glad/src/glad.c`.
3. Compiles `Resource Files/opengl.cpp`.
4. Links `build/bin/opengl.exe`.
5. Copies runtime DLLs and assets into `build/bin`.
6. Launches `build/bin/opengl.exe`.

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

### Map Export Notes

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

### Troubleshooting

If PowerShell blocks the script, use the exact command from the run section:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

If `g++.exe` is missing, install MSYS2 and confirm this file exists:

```text
C:\msys64\mingw64\bin\g++.exe
```

If GLFW or GLEW are missing, install them:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

## Español

Guia rapida para compilar y ejecutar el proyecto en Windows 10.

### Ejecutar El Proyecto

Abre PowerShell en la raiz del proyecto y ejecuta este comando:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

Esta es la forma prevista para correr el proyecto. Recompila el codigo, copia los assets y DLLs necesarios a `build/bin`, y abre el juego inmediatamente, asi que los ultimos cambios siempre se ven reflejados.

### Requisitos

- Windows 10 o Windows 11.
- GPU/driver con soporte para OpenGL 3.3.
- MSYS2 instalado en `C:\msys64`.
- MinGW 64-bit `g++.exe`.
- Paquetes MinGW de GLFW y GLEW.

Instala los paquetes necesarios desde la terminal `MSYS2 MinGW x64`:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

El script busca automaticamente:

```text
C:\msys64\mingw64\bin\g++.exe
```

### Estructura Del Proyecto

```text
.
|-- Resource Files/
|   |-- opengl.cpp                  Punto de entrada principal
|   |-- systems/                    Modulos de gameplay/UI/audio
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
|   |-- *.vs / *.fs                 Shaders GLSL
|   |-- shwallpaper.jpeg            Fondo del menu
|   `-- video_sh.wmv                Cinematica inicial
|-- models/
|   |-- town_visual.glb             Mapa principal
|   |-- james/                      Modelo, sonidos y armas de James
|   |-- jamesanimations/            Animaciones de James
|   |-- angela/                     Modelo, animaciones, sonidos y video de Angela
|   `-- enemies/                    Pyramid Head y otros enemigos
|-- sounds/                         Musica y efectos compartidos
|-- skybox/                         Texturas del skybox
|-- glad/                           Loader de OpenGL
|-- glm/                            Libreria matematica
|-- SOIL2/                          Helpers para cargar imagenes
|-- assimp/                         Headers/build outputs de Assimp
|-- .vscode/                        Tareas opcionales de VS Code
|-- build.ps1                       Script para compilar y ejecutar
|-- build.sh                        Helper de shell
|-- CMakeLists.txt                  Proyecto CMake
|-- package.ps1                     Helper de empaquetado
`-- README.md
```

### Que Hace `build.ps1`

1. Busca `g++.exe`.
2. Compila `glad/src/glad.c`.
3. Compila `Resource Files/opengl.cpp`.
4. Enlaza `build/bin/opengl.exe`.
5. Copia DLLs y assets a `build/bin`.
6. Ejecuta `build/bin/opengl.exe`.

Contenido copiado:

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

### Notas Para Exportar El Mapa

El mapa principal es:

```text
models/town_visual.glb
```

El motor lee estos nodos del mapa:

- `spawn_player`: posicion y direccion inicial de James.
- `angelainitialpos`: posicion inicial de Angela.
- `shotgunpos`: posicion de la escopeta.
- `savepoint*`: marcadores de guardado.
- `walkarea*`: superficies de navegacion.
- `lightpos*`: posiciones de luces.

Conserva esos nombres al exportar el GLB otra vez.

### Problemas Comunes

Si PowerShell bloquea el script, usa el comando exacto de la seccion de ejecucion:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

Si falta `g++.exe`, instala MSYS2 y confirma que exista:

```text
C:\msys64\mingw64\bin\g++.exe
```

Si faltan GLFW o GLEW, instalalos:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```
