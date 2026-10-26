cmake_minimum_required(VERSION 3.16)
project(funny LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FUNNY_SANITIZE "Build with ASan/UBSan" OFF)

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

add_subdirectory(automata)
add_subdirectory(lexspec)
add_subdirectory(token)
add_subdirectory(lexer)
add_subdirectory(ast)
add_subdirectory(parser)

add_executable(main_parser
    main_parser.cpp)
target_link_libraries(main_parser PUBLIC lexer parser)
target_compile_features(main_parser PUBLIC cxx_std_20)
target_link_libraries(main_parser PRIVATE funny_options)

add_executable(parser_cases tests/tests.cpp)
target_link_libraries(parser_cases PRIVATE parser ast token funny_options)
add_test(NAME parser_cases COMMAND parser_cases)