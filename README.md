## csm_cmd

Безопасный терминальный обработчик команд для режима `-nogui` на базе replxx.

### Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Опции:

- `-DBUILD_TERMINAL_TESTS=ON|OFF` — сборка тестов (по умолчанию `ON`).

Требования: CMake >= 3.16, компилятор с поддержкой C++17, доступ к сети при
конфигурации (FetchContent скачивает replxx и googletest).

### Запуск

```bash
./build/csm_cmd_app -nogui
./build/csm_cmd_app -nogui --timeout-ms 200
```

Внутри терминала доступны команды: `help`, `history`, `clear`, `quit`,
`echo`, `version`, `time`, а также демонстрационные `greet`, `list` (алиас
`ls`) и `slow` (пример команды, превышающей таймаут).

### Тесты

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Интеграция в другой проект

Через `add_subdirectory`:

```cmake
add_subdirectory(third_party/csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```

Через `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  csm_cmd_project
  GIT_REPOSITORY <todo add url>
  GIT_TAG        main
)
FetchContent_MakeAvailable(csm_cmd)
target_link_libraries(your_app PRIVATE csm_cmd)
```

Пример использования в коде:

```cpp
#include "csm_cmd/csm_terminal.hpp"

int main()
{
  csm_cmd::csm_terminal terminal;

  terminal.registerCommand("greet", [](const std::vector<std::string>& args)
  {
    std::cout << "Hello, " << (args.empty() ? "World" : args[0]) << "!\n";
    return 0;
  }, "greet [name] - Say hello");

  terminal.run();
  return 0;
}
```

### Безопасность

- Максимальная длина ввода — 4096 символов (`CommandParser::kMaxInputLength`).
- Выполняются только явно зарегистрированные команды (`CommandRegistry`) —
  нет вызовов `system()`/`exec*()`.
- Вывод в терминал экранируется (`csm_terminal::escapeOutput`) для защиты от
  инъекции управляющих ANSI-последовательностей.
- Каждая команда выполняется с таймаутом на базе `std::chrono::steady_clock`
  (по умолчанию 100 мс, настраивается через конструктор `csm_terminal`).
- Логи пишутся в `~/.csm_cmd.log` с ротацией по размеру (макс. 1 МБ, до 3
  резервных копий).
- История команд сохраняется в `~/.csm_cmd_history`, ограничена 1000
  строками (`csm_terminal::kMaxHistoryLines`).
- `Ctrl+C` (`SIGINT`) обрабатывается корректно — история сохраняется, ресурсы
  освобождаются, приложение завершается штатно.
