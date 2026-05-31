param(
    [switch]$NoRun
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

function Invoke-Native {
    param(
        [scriptblock]$Command,
        [string]$FailureMessage
    )

    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }
}

$gxxCandidates = @(
    'C:\msys64\mingw64\bin\g++.exe',
    'C:\msys64\usr\bin\g++.exe'
)

$gxx = $null
foreach ($candidate in $gxxCandidates) {
    if (Test-Path $candidate) {
        $gxx = $candidate
        break
    }
}

if (-not $gxx) {
    $command = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($command) {
        $gxx = $command.Source
    }
}

if (-not $gxx) {
    throw 'No se encontro g++.exe. Instala MSYS2/MinGW o ajusta la ruta en build.ps1.'
}

New-Item -ItemType Directory -Force -Path 'build/bin', 'build/obj' | Out-Null

Write-Host 'Compiling glad...'
Invoke-Native { & $gxx -c 'glad/src/glad.c' -o 'build/obj/glad.o' -I'glad/src/include' } 'Failed to compile glad.c'

Write-Host 'Compiling opengl.cpp...'
Invoke-Native {
    & $gxx -std=c++17 -c 'Resource Files/opengl.cpp' -o 'build/obj/opengl.o' `
        -I'glad/src/include' -I'.' -I'Resource Files' -I'assimp/include'
} 'Failed to compile Resource Files/opengl.cpp'

Write-Host 'Linking...'
$linkArgs = @(
    '-std=c++17'
    'build/obj/glad.o'
    'build/obj/opengl.o'
    '-o'
    'build/bin/opengl.exe'
    '-lopengl32'
    '-lgdi32'
    '-luser32'
    '-lshell32'
    '-lwinmm'
    '-lgdiplus'
    '-lglfw3'
    '-lglew32'
    '-Lassimp/build-mingw/lib'
    '-lassimp'
    '-L.'
    '-Wl,--allow-shlib-undefined'
)
Invoke-Native { & $gxx @linkArgs } 'Link step failed'

Write-Host 'Copying DLLs and resources...'
if (Test-Path 'glfw3.dll') { Copy-Item 'glfw3.dll' 'build/bin/' -Force }
if (Test-Path 'glew32.dll') { Copy-Item 'glew32.dll' 'build/bin/' -Force }
if (Test-Path 'assimp/build-mingw/bin/libassimp-6.dll') {
    Copy-Item 'assimp/build-mingw/bin/libassimp-6.dll' 'build/bin/' -Force
} else {
    Write-Host 'Warning: libassimp-6.dll not found'
}

foreach ($runtimeDll in @('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
    $runtimePath = Join-Path (Split-Path -Parent $gxx) $runtimeDll
    if (Test-Path $runtimePath) {
        Copy-Item $runtimePath 'build/bin/' -Force
    }
}

New-Item -ItemType Directory -Force -Path 'build/bin/Resource Files' | Out-Null
Copy-Item 'Resource Files/*.vs', 'Resource Files/*.fs' 'build/bin/Resource Files/' -Force
if (Test-Path 'build/bin/skybox') { Remove-Item 'build/bin/skybox' -Recurse -Force }
if (Test-Path 'build/bin/models') { Remove-Item 'build/bin/models' -Recurse -Force }
if (Test-Path 'skybox') { Copy-Item 'skybox' 'build/bin/' -Recurse -Force }
if (Test-Path 'models') { Copy-Item 'models' 'build/bin/' -Recurse -Force }

Write-Host 'Done! Executable: build/bin/opengl.exe'
Get-Item 'build/bin/opengl.exe' | Format-List FullName, Length, LastWriteTime

if (-not $NoRun) {
    Write-Host 'Launching build/bin/opengl.exe...'
    & '.\build\bin\opengl.exe'
}
