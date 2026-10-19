add_library(lexspec
    src/ast.cpp
    src/parser.cpp)
target_include_directories(lexspec PUBLIC include)
target_compile_features(lexspec PUBLIC cxx_std_20)
target_link_libraries(lexspec PRIVATE funny_options)