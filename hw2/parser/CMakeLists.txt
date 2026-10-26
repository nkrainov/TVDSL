add_library(parser 
    src/parser.cpp)
target_include_directories(parser PUBLIC include)
target_link_libraries(parser PUBLIC ast token)
target_compile_features(parser PUBLIC cxx_std_20)
target_link_libraries(parser PRIVATE funny_options)