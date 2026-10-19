add_library(lexer
    src/lexer.cpp)
target_include_directories(lexer PUBLIC include)
target_compile_features(lexer PUBLIC cxx_std_20)
target_link_libraries(lexer PUBLIC automata)
target_link_libraries(lexer PRIVATE funny_options)
