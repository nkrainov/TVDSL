add_library(ast
    src/ast.cpp
    src/print.cpp
    src/equals.cpp)
target_include_directories(ast PUBLIC include)
target_compile_features(ast PUBLIC cxx_std_20)
target_link_libraries(ast PRIVATE funny_options)