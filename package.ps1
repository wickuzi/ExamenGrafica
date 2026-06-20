$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$sourceBin = Join-Path $root 'build/bin'
$exe = Join-Path $sourceBin 'opengl.exe'
$dist = Join-Path $root 'dist/opengl-portable'

if (-not (Test-Path $exe)) {
    Write-Host 'build/bin/opengl.exe was not found. Building first...'
    & (Join-Path $root 'build.ps1') -NoRun
}

if (-not (Test-Path $exe)) {
    throw 'Could not create build/bin/opengl.exe.'
}

New-Item -ItemType Directory -Force -Path $dist | Out-Null

Copy-Item $exe $dist -Force

foreach ($file in @('glfw3.dll', 'glew32.dll', 'libassimp-6.dll', 'libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
    $candidates = @(
        (Join-Path $sourceBin $file),
        (Join-Path $root $file),
        (Join-Path $root "assimp/build-mingw/bin/$file")
    )

    $found = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($found) {
        Copy-Item $found $dist -Force
    } else {
        Write-Host "Warning: $file not found"
    }
}

New-Item -ItemType Directory -Force -Path (Join-Path $dist 'Resource Files') | Out-Null
Copy-Item 'Resource Files/*.vs', 'Resource Files/*.fs' (Join-Path $dist 'Resource Files') -Force

foreach ($dir in @('models', 'skybox')) {
    $source = Join-Path $root $dir
    $target = Join-Path $dist $dir
    if (Test-Path $source) {
        if (Test-Path $target) { Remove-Item $target -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $target | Out-Null
        Copy-Item (Join-Path $source '*') $target -Recurse -Force
    } else {
        Write-Host "Warning: $dir not found"
    }
}

Write-Host "Portable folder ready: $dist"
Get-ChildItem $dist | Select-Object Name, Length, LastWriteTime
