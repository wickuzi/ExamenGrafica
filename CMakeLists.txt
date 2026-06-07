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
