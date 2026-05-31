# Paseo Virtual Silent Hill 2 con OpenGL

Proyecto de la materia Programación Gráfica. Paseo Virtual basado en el videojuego clásico **Silent Hill 2**, realizado en OpenGL. Esta configurado para compilarse y ejecutarse desde **Visual Studio Code en Windows**.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3-green)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![IDE](https://img.shields.io/badge/IDE-VS%20Code-blueviolet)

## Tabla De Contenido

- [Caracteristicas](#caracteristicas)
- [Estructura Del Proyecto](#estructura-del-proyecto)
- [Requisitos](#requisitos)
- [Instalacion En Una Laptop Nueva](#instalacion-en-una-laptop-nueva)
- [Abrir En Visual Studio Code](#abrir-en-visual-studio-code)
- [Compilar](#compilar)
- [Ejecutar](#ejecutar)
- [Crear Version Portable](#crear-version-portable)
- [Solucion De Problemas](#solucion-de-problemas)

## Caracteristicas

- Renderizado 3D con OpenGL.
- Modelo principal: **James Sunderland**.
- Skybox cubico con seis texturas.
- Fog gris verdosa y tono visual inspirado en Silent Hill 2.
- Iluminacion Phong con luz direccional y luces puntuales.
- Camara en tercera persona con rotacion por mouse.
- Carga de modelos `.glb` y `.dae` mediante Assimp.
- Scripts de build para PowerShell y Bash.
- Configuracion lista para VS Code.
- Empaquetado portable para ejecutar en otra laptop.

## Estructura Del Proyecto

```text
.
|-- Resource Files/        # Codigo fuente, shaders y headers locales
|-- models/
|   |-- james/             # Modelo 3D de James Sunderland
|   `-- map/               # Escenario Collada y texturas
|-- skybox/                # Texturas del cubemap
|-- glad/                  # Loader de OpenGL
|-- glm/                   # Libreria matematica
|-- SOIL2/                 # stb_image y helpers de imagen
|-- assimp/                # Assimp y librerias compiladas
|-- .vscode/               # Tareas, debug e IntelliSense para VS Code
|-- build.ps1              # Compila en Windows con PowerShell
|-- build.sh               # Compila desde shell tipo MSYS2/Git Bash
|-- package.ps1            # Genera dist/opengl-portable
`-- CMakeLists.txt         # Configuracion CMake alternativa
```

Carpetas generadas:

```text
build/                  # Resultado de compilacion local
dist/opengl-portable/   # Carpeta portable para compartir
```

Estas carpetas se pueden borrar y regenerar.

## Requisitos

- Windows 10/11.
- Visual Studio Code.
- Extension **C/C++** de Microsoft para VS Code.
- MSYS2/MinGW 64-bit instalado en `C:\msys64`.
- Driver de video con soporte para **OpenGL 3.3** o superior.

El proyecto ya incluye `glad`, `glm`, `SOIL2`, `assimp`, shaders, skybox, el modelo 3D y las DLLs principales. No muevas esas carpetas fuera del proyecto.

## Instalacion En Una Laptop Nueva

1. Instala **MSYS2** desde su sitio oficial.
2. Abre la terminal **MSYS2 MinGW x64**.
3. Instala compilador, debugger y librerias:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

4. Verifica que exista:

```text
C:\msys64\mingw64\bin\g++.exe
```

## Abrir En Visual Studio Code

1. Abre VS Code.
2. Ve a `File > Open Folder...`.
3. Selecciona la carpeta completa del proyecto.
4. No abras solamente `Resource Files`, porque VS Code necesita ver `.vscode`, `models`, `skybox` y los scripts.

Tareas disponibles:

| Tarea | Uso |
| --- | --- |
| `Build OpenGL` | Compila sin ejecutar. |
| `Run OpenGL` | Compila y ejecuta. |
| `Package Portable` | Crea `dist/opengl-portable`. |
| `Debug OpenGL` | Ejecuta con `F5` usando `gdb`. |

## Compilar

### Desde VS Code

Presiona:

```text
Ctrl+Shift+B
```

Si VS Code pregunta que tarea ejecutar, elige:

```text
Build OpenGL
```

Esto ejecuta internamente:

```powershell
.\build.ps1 -NoRun
```

El ejecutable queda en:

```text
build/bin/opengl.exe
```

Durante la compilacion se copian automaticamente a `build/bin`:

- `Resource Files` con los shaders necesarios.
- `models` con James Sunderland.
- `skybox` con las seis texturas.
- `glfw3.dll`.
- `glew32.dll`.
- `libassimp-6.dll`.
- DLLs runtime de MinGW cuando estan disponibles.

### Desde PowerShell

Desde la raiz del proyecto:

```powershell
.\build.ps1 -NoRun
```

Si Windows bloquea la ejecucion de scripts:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1 -NoRun
```

## Ejecutar

### Desde VS Code

Opcion recomendada:

```text
Terminal > Run Task... > Run OpenGL
```

Para depurar:

```text
F5 > Debug OpenGL
```

### Desde PowerShell

Compilar y ejecutar en un solo paso:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\build.ps1
```

Ejecutar manualmente:

```powershell
cd .\build\bin
.\opengl.exe
```

El programa debe ejecutarse desde `build/bin` porque carga shaders, modelo y skybox usando rutas relativas al `.exe`.

## Controles

| Control | Accion |
| --- | --- |
| `W`, `A`, `S`, `D` | Mover a James por la escena. |
| Mouse | Rotar la camara en tercera persona. |
| Rueda del mouse | Acercar o alejar la camara. |
| `Shift` | Movimiento mas rapido. |
| `I`, `J`, `K`, `L`, `U`, `O` | Ajustar la luz blanca de prueba. |
| `Esc` | Cerrar la ventana. |

## Crear Version Portable

Para crear una carpeta lista para compartir:

```powershell
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\package.ps1
```

Se genera:

```text
dist/opengl-portable
```

Contenido esperado:

```text
opengl.exe
glfw3.dll
glew32.dll
libassimp-6.dll
libstdc++-6.dll
libgcc_s_seh-1.dll
libwinpthread-1.dll
Resource Files/
models/
skybox/
```

Para compartirlo, comprime `dist/opengl-portable` y abre `opengl.exe` desde esa misma carpeta en la otra laptop.

## Solucion De Problemas

### `g++.exe` No Se Encuentra

Verifica que exista:

```text
C:\msys64\mingw64\bin\g++.exe
```

Si no existe, instala MSYS2 y los paquetes indicados en [Instalacion En Una Laptop Nueva](#instalacion-en-una-laptop-nueva).

### Error `cannot find -lglfw3` O `cannot find -lglew32`

Instala las librerias faltantes desde **MSYS2 MinGW x64**:

```bash
pacman -S --needed mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

### Error Con `gdb.exe` Al Presionar F5

Instala el debugger:

```bash
pacman -S --needed mingw-w64-x86_64-gdb
```

### El Skybox No Carga

Verifica que las seis imagenes esten en:

```text
build/bin/skybox
```

o, en la version portable:

```text
dist/opengl-portable/skybox
```

### El Programa Compila Pero No Abre

Revisa lo siguiente:

- `opengl.exe` debe estar junto a sus DLLs.
- `Resource Files`, `models` y `skybox` deben estar junto al `.exe`.
- El driver de video debe estar actualizado.
- La GPU debe soportar OpenGL 3.3 o superior.

Para ver errores en consola:

```powershell
cd .\build\bin
.\opengl.exe
```
