cmake_minimum_required(VERSION 3.22)
project(OpenGL_Aliasing CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Glad library
add_library(glad STATIC "glad/src/glad.c")
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/include)

# Main executable
add_executable(opengl "Resource Files/opengl.cpp")

# Implementation modules included by opengl.cpp. Listing them here keeps them
# visible and grouped in IDEs without changing the proven single translation
# unit or introducing global-state initialization order changes.
target_sources(opengl PRIVATE
  "Resource Files/systems/audio_system.inl"
  "Resource Files/systems/damage_overlay.inl"
  "Resource Files/systems/enemy_system.inl"
  "Resource Files/systems/flashlight_system.inl"
  "Resource Files/systems/texture_system.inl"
  "Resource Files/systems/navigation_system.inl"
  "Resource Files/systems/geometry_system.inl"
  "Resource Files/systems/model_animation_system.inl"
  "Resource Files/systems/angela_system.inl"
  "Resource Files/systems/health_system.inl"
  "Resource Files/systems/objective_overlay.inl"
  "Resource Files/systems/objectives_hud.inl"
  "Resource Files/systems/status_hud.inl"
  "Resource Files/systems/shooting_system.inl"
  "Resource Files/systems/pause_menu.inl"
)

target_include_directories(opengl PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/include
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/SOIL2
  ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files"
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# Link with all necessary Windows libraries
target_link_libraries(opengl PRIVATE
  glad
  opengl32
  gdi32
  user32
  shell32
  winmm
  gdiplus
  ole32
  oleaut32
  uuid
  strmiids
  glfw3
  glew32
  -Wl,--allow-shlib-undefined
)

# Copy DLLs and runtime resources
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resource Files"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/2.2.basic_lighting.vs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/2.2.basic_lighting.fs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/2.2.light_cube.vs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/2.2.light_cube.fs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/skybox.vs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/skybox.fs"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/video_sh.wmv"
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource Files/shwallpaper.jpeg"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resource Files"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/skybox"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/skybox"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/models"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/sounds"
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sounds"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/glfw3.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glfw3.dll"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/glew32.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glew32.dll"
)
add_custom_command(TARGET opengl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/assimp/build-mingw/bin/libassimp-6.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libassimp-6.dll"
)
get_filename_component(CXX_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
foreach(MINGW_RUNTIME_DLL libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
  if(EXISTS "${CXX_COMPILER_DIR}/${MINGW_RUNTIME_DLL}")
    add_custom_command(TARGET opengl POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CXX_COMPILER_DIR}/${MINGW_RUNTIME_DLL}"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MINGW_RUNTIME_DLL}"
    )
  endif()
endforeach()
