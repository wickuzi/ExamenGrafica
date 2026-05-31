#!/bin/bash

set -e

cd "$(dirname "$0")"

mkdir -p build/bin build/obj

echo "Compiling glad..."
g++ -c glad/src/glad.c -o build/obj/glad.o -Iglad/src/include

echo "Compiling opengl.cpp..."
g++ -std=c++17 -c "Resource Files/opengl.cpp" -o build/obj/opengl.o \
  -Iglad/src/include -I. -I"Resource Files" -Iassimp/include

echo "Linking..."
g++ -std=c++17 build/obj/glad.o build/obj/opengl.o -o build/bin/opengl.exe \
  -lopengl32 -lgdi32 -luser32 -lshell32 -lwinmm -lgdiplus -lglfw3 -lglew32 \
  -L. -Lassimp/build-mingw/lib -Lassimp/build-mingw/code/CMakeFiles/assimp.dir \
  -lassimp -Wl,--allow-shlib-undefined

echo "Copying DLLs and resources..."
cp glfw3.dll glew32.dll build/bin/
cp "assimp/build-mingw/bin/libassimp-6.dll" build/bin/ 2>/dev/null || echo "Warning: libassimp-6.dll not found"
mkdir -p "build/bin/Resource Files"
cp "Resource Files"/*.vs "Resource Files"/*.fs "build/bin/Resource Files/"
cp -r skybox build/bin/ 2>/dev/null || true
cp -r models build/bin/ 2>/dev/null || true

echo "Done! Executable: build/bin/opengl.exe"
ls -lh build/bin/opengl.exe

echo "Launching build/bin/opengl.exe..."
./build/bin/opengl.exe
