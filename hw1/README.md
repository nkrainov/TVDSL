Сборка проекта

`
    cmake -S . -B build
    cmake --build build
`

Запуск билдера ДКА, НКА и минимального ДКА
`
    ./build/funny_builder tokens.txt -o data
`
Запуск раннера лексера, прогоняющего строки из cases.txt с помощью построенного ДКА

`
    ./build/funny_runner data/dfa_min.txt cases.txt
`

В НКА получилось 295 состояний, в ДКА 125, в минимальном ДКА 122 состояния.
