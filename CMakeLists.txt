cmake_minimum_required(VERSION 3.20)
project(metafield-engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

# Core library
add_library(metafield_core
  engine/core/types.cpp
  engine/ecs/entity.cpp
  engine/ecs/registry.cpp
  engine/world/world.cpp
  engine/world/time.cpp
  engine/fields/field.cpp
)

target_include_directories(metafield_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Minimal smoke test executable
add_executable(hello_world examples/hello-world/main.cpp)
target_link_libraries(hello_world PRIVATE metafield_core)

enable_testing()
add_test(NAME hello_world COMMAND hello_world)
