cmake_minimum_required(VERSION 3.20)
project(metafield-engine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(metafield_core
  engine/core/types.cpp
  engine/ecs/entity.cpp
  engine/ecs/registry.cpp
  engine/world/world.cpp
  engine/world/time.cpp
  engine/fields/field.cpp
  engine/fields/csi_field.cpp
  engine/ingest/csi_parse.cpp
)

target_include_directories(metafield_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(hello_world examples/hello-world/main.cpp)
target_link_libraries(hello_world PRIVATE metafield_core)

add_executable(hello_csi examples/hello-csi/main.cpp)
target_link_libraries(hello_csi PRIVATE metafield_core)

enable_testing()
add_test(NAME hello_world COMMAND hello_world)
add_test(NAME hello_csi_synth COMMAND hello_csi --synth --seconds 2)
