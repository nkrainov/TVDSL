add_library(automata
    src/dfa.cpp
    src/dfa_serialization.cpp
    src/nfa.cpp
    src/nfa_serialization.cpp
    )

target_include_directories(automata PUBLIC include)
target_compile_features(automata PUBLIC cxx_std_20)
target_link_libraries(automata PRIVATE funny_options)