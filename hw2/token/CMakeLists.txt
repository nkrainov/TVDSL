add_library(token
    src/tokentable.cpp)
target_include_directories(token PUBLIC include)
target_compile_features(token PUBLIC cxx_std_20)
target_link_libraries(token PRIVATE funny_options)
