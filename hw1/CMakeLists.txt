cmake_minimum_required(VERSION 3.16)
project(funny_lexer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FUNNY_SANITIZE "Build with ASan/UBSan" OFF)

# Общие предупреждения и санитайзеры: подключается к целям через target_link_libraries.
add_library(funny_options INTERFACE)
if(MSVC)
  target_compile_options(funny_options INTERFACE /W4)
else()
  target_compile_options(funny_options INTERFACE -Wall -Wextra -Wpedantic)
  if(FUNNY_SANITIZE)
    target_compile_options(funny_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(funny_options INTERFACE -fsanitize=address,undefined)
  endif()
endif()

#enable_testing()

add_subdirectory(automata)
add_subdirectory(lexspec)
add_subdirectory(builder)
add_subdirectory(lexer)

add_executable(funny_builder main_builder.cpp)
target_link_libraries(funny_builder PRIVATE builder funny_options)

add_executable(funny_runner main_runner.cpp)
target_link_libraries(funny_runner PRIVATE lexer funny_options)
