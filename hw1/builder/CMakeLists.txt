add_library(builder
    src/thompson.cpp
    src/subset.cpp
    src/minimize.cpp)
target_include_directories(builder PUBLIC include)
target_compile_features(builder PUBLIC cxx_std_20)
target_link_libraries(builder PUBLIC automata lexspec)
target_link_libraries(builder PRIVATE funny_options)