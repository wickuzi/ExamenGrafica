# Paseo Virtual Silent Hill 2 con OpenGL

Guia para compilar y ejecutar el proyecto correctamente en una computadora Windows.

## Requisitos

- Windows 10 o Windows 11.
- Driver de video actualizado con soporte para OpenGL 3.3 o superior.
- Visual Studio Code, opcional pero recomendado.
- MSYS2 instalado en `C:\msys64`.
- Compilador MinGW 64-bit.
- Librerias MinGW de GLFW y GLEW.

El proyecto ya incluye las carpetas y archivos principales del motor:

```text
Resource Files/   codigo fuente y shaders
models/           modelos 3D y texturas del mapa/personaje
sounds/           musica y efectos de sonido
skybox/           texturas del cielo
glad/             loader de OpenGL
glm/              libreria matematica
SOIL2/            carga de imagenes
assimp/           headers y libreria compilada de Assimp
glfw3.dll
glew32.dll
build.ps1
```

No muevas ni borres esas carpetas. El ejecutable necesita que los recursos se copien junto a `opengl.exe`.

## Instalar MSYS2 y Dependencias

1. Instala MSYS2 desde:

```text
https://www.msys2.org/
```

2. Abre la terminal:

```text
MSYS2 MinGW x64
```

3. Instala el compilador y librerias necesarias:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

4. Verifica que exista el compilador:

```text
C:\msys64\mingw64\bin\g++.exe
```

Si esa ruta existe, el script `build.ps1` lo detecta automaticamente.

## Abrir el Proyecto

Abre la carpeta completa del proyecto, no solo `Resource Files`.

La raiz debe verse parecido a esto:

```text
aliasing/
|-- Resource Files/
|-- models/
|-- sounds/
|-- skybox/
|-- glad/
|-- glm/
|-- SOIL2/
|-- assimp/
|-- build.ps1
|-- CMakeLists.txt
`-- README.md
```

## Compilar

Desde PowerShell, entra a la carpeta raiz del proyecto:

```powershell
cd C:\ruta\al\proyecto\aliasing
```

Compila sin ejecutar:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

Si todo sale bien, el ejecutable queda en:

```text
build/bin/opengl.exe
```

Durante la compilacion el script copia automaticamente:

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

## Ejecutar

Opcion recomendada:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

Eso compila y luego abre el programa.

Tambien puedes ejecutarlo manualmente:

```powershell
cd .\build\bin
.\opengl.exe
```

Importante: ejecuta `opengl.exe` desde `build/bin`, porque el programa carga shaders, modelos, sonidos y skybox usando rutas relativas.

## Compilar Desde VS Code

1. Abre la carpeta completa del proyecto en VS Code.
2. Presiona:

```text
Ctrl+Shift+B
```

3. Selecciona la tarea:

```text
Build OpenGL
```

Para ejecutar desde VS Code, usa:

```text
Terminal > Run Task... > Run OpenGL
```

## Problemas Comunes

### PowerShell bloquea el script

Usa siempre este comando:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

### No se encuentra `g++.exe`

Instala MSYS2 y confirma que exista:

```text
C:\msys64\mingw64\bin\g++.exe
```

### Error `cannot find -lglfw3` o `cannot find -lglew32`

Instala GLFW y GLEW desde la terminal `MSYS2 MinGW x64`:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

### Falta `libassimp-6.dll`

Verifica que exista:

```text
assimp/build-mingw/bin/libassimp-6.dll
```

El script la copia a:

```text
build/bin/libassimp-6.dll
```

### El juego abre pero no carga modelos, sonidos o skybox

Revisa que existan estas carpetas:

```text
build/bin/Resource Files
build/bin/models
build/bin/sounds
build/bin/skybox
```

Si falta alguna, vuelve a compilar con:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

### El programa no abre

Ejecutalo desde consola para ver el error:

```powershell
cd .\build\bin
.\opengl.exe
```

Tambien revisa que tu GPU soporte OpenGL 3.3 o superior.
