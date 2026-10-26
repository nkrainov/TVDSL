Парсер - LL(1)

Сборка проекта

`
    cmake -S . -B build
    cmake --build build
`

Запуск примера парсера (по ДКА для лексера и примеру кода выводит текстовое представление AST)
`
    ./build/main_parser data/dfa_min.txt example.funny
`
Запуск тестов (сами кейсы в parser_cases.hpp)
`
  ./build/parser_cases -v
`

Пример вывода для примера из example.funny
```
function minmax(a: int[]) returns (mn: int, mx: int)
  requires (> (call length a) 0)
  block
    assign mn = (index a 0)
    assign mx = (index a 0)
    assign i = 1
    while (< i (call length a))
      block
        if (< (index a i) mn)
          assign mn = (index a i)
        if (> (index a i) mx)
          assign mx = (index a i)
        assign i = (+ i 1)
function normalize(a: int[]) returns (b: int[])
  requires (>= (call length a) 2)
  block
    assign b = a
    call-assign mn, mx = (call minmax b)
    if (!= mx mn)
      block
        assign b[0] = mn
        assign b[1] = mx
```

Если там специально оставить ошибку (убрать ;), то вывод будет примерно такой:
```
example.funny:15:5: error: expected ';', got 'if'
function minmax(a: int[]) returns (mn: int, mx: int)
  requires (> (call length a) 0)
  block
    assign mn = (index a 0)
    assign mx = (index a 0)
    assign i = 1
    while (< i (call length a))
      block
        if (< (index a i) mn)
          assign mn = (index a i)
        if (> (index a i) mx)
          assign mx = (index a i)
        assign i = (+ i 1)
function normalize(a: int[]) returns (b: int[])
  requires (>= (call length a) 2)
  block
    assign b = a
    <error>
    if (!= mx mn)
      block
        assign b[0] = mn
        assign b[1] = mx
```