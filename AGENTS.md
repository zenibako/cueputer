# Cueputer - Agent Guidelines

Theater tech multitool firmware for the M5Stack Cardputer ADV (ESP32-S3).
Built with PlatformIO + Arduino framework in C++. Header-only architecture.

## Build Commands

```bash
pio run                        # Build firmware
pio run -t upload              # Build and flash to device via USB
pio run -t clean               # Clean build artifacts
pio device monitor             # Serial monitor (115200 baud)
pio run -t upload && pio device monitor  # Flash + monitor
```

## Testing

No test suite exists yet. When adding tests, follow this structure:

```
test/
  native/           # Pure-logic tests that run on the host (CI-runnable)
    test_parser/
      test_main.cpp
  embedded/          # Hardware-dependent tests (device only)
    test_display/
      test_main.cpp
```

Tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) framework (PlatformIO default).
Add a `[env:native]` section to `platformio.ini` for host-runnable tests:

```ini
[env:native]
platform = native
test_filter = native/*
```

```bash
pio test -e native                          # Run all native tests
pio test -e native -f "native/test_parser"  # Run a single test suite
pio test --list-tests                       # List available tests
pio test -e native --junit-output-path report.xml  # JUnit output for CI
```

## Static Analysis (Linting)

```bash
pio check --skip-packages                               # Cppcheck (default)
pio check --fail-on-defect=high --skip-packages          # Fail on high-severity
pio check --fail-on-defect=medium --skip-packages        # Fail on medium+ severity
```

Always use `--skip-packages` to avoid analyzing third-party framework internals.

## CI / GitHub Actions

The workflow at `.github/workflows/ci.yml` runs on push/PR to `main`:
- **build**: Compiles firmware for the Cardputer ADV target
- **static-analysis**: Runs `pio check` (cppcheck), fails on medium+ defects
- **native-tests**: Runs `pio test -e native` (once a `[env:native]` and tests exist)

PlatformIO platforms/packages are cached keyed on `platformio.ini` hash.

## Code Style

### Architecture

Header-only codebase. All code lives in `.h` files with `inline` public functions
and `static` internal functions/state. The only `.cpp` file is `src/main.cpp`.

Each module is a **namespace acting as a singleton module**:
```cpp
namespace ModuleName {
    static const int SOME_CONST = 42;    // constants
    struct SomeType { ... };              // types
    static WiFiUDP _udp;                 // module state (underscore prefix)
    static void helperFunc() { ... }     // internal helpers (static)
    inline void run() { ... }            // public entry point (inline)
} // namespace ModuleName
```

No inheritance, no virtual methods, no interfaces. Pure procedural code in namespaces.

### File Layout

```
src/
  main.cpp              # Entry point (setup/loop)
  config/settings.h     # NVS persistence
  ui/display.h          # LCD drawing primitives
  ui/keyboard.h         # Keyboard input abstraction
  ui/menu.h             # Reusable menu system
  net/wifi_manager.h    # WiFi management with UI
  modules/<name>/<name>.h   # One namespace per module
```

### Naming Conventions

| Element            | Style                | Example                          |
|--------------------|----------------------|----------------------------------|
| Files/directories  | `snake_case`         | `osc_tester.h`, `wifi_manager/`  |
| Namespaces         | `PascalCase`         | `OscTester`, `WiFiManager`       |
| Classes/structs    | `PascalCase`         | `MenuScreen`, `OscArg`           |
| Functions          | `camelCase`          | `connectSaved()`, `drawHistory()`|
| Member variables   | `_camelCase`         | `_targetHost`, `_selected`       |
| Local variables    | `camelCase`          | `startTime`, `rssiTmp`           |
| Constants          | `SCREAMING_SNAKE`    | `COLOR_BG`, `MAX_TEMPLATES`      |
| Enum values        | `SCREAMING_SNAKE`    | `ARG_INT`, `ARG_FLOAT`           |
| NVS key strings    | `"snake_case"`       | `"wifi_ssid"`, `"osc_host"`      |
| Type aliases       | `using` (not typedef)| `using Action = std::function<void()>;` |

### Formatting

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Header guards**: `#pragma once` (no `#ifndef` guards)
- **Line width**: No strict limit, but keep reasonable (~100 chars)
- Single-statement `if` on same line only for short returns: `if (x) return false;`
- Multi-statement bodies always use braces

### Includes

Three groups separated by blank lines:
1. Platform/framework + third-party (angle brackets): `<M5Unified.h>`, `<WiFi.h>`, `<vector>`
2. Project-local (double quotes, relative paths): `"../../ui/display.h"`

```cpp
#include <M5Unified.h>
#include <WiFi.h>
#include <vector>

#include "ui/display.h"
#include "config/settings.h"
```

### Comments

Only `//` line comments. No `/* */` blocks, no doxygen.

```cpp
// ============================================================
//  Section header (box style for major sections)
// ============================================================

// --- Sub-section header -------------------------------------------

int x = 0; // inline explanation
```

### Types

- Prefer explicit types over `auto`
- `auto` is acceptable for: M5 config init, lambdas, range-for with complex types
- Enums use explicit base types: `enum Key : uint8_t { ... };`
- Use `String` for dynamic text, `const char*` for static strings, `snprintf` for formatting

### Error Handling

No exceptions. No `Serial.println` debugging. Three patterns:

1. **Bool returns** for fallible operations: `bool connectSaved() { ... return false; }`
2. **Display feedback** with color-coded status: `Display::status("Failed", Display::COLOR_ERROR);`
3. **Guard clauses** with silent early return: `if (!ready) return;`

Color severity: `COLOR_OK` (green), `COLOR_WARN` (amber), `COLOR_ERROR` (red), `COLOR_MUTED` (gray).

### UI Event Loop Pattern

Every screen follows this blocking loop:
```cpp
while (true) {
    Keyboard::Event ev = Keyboard::poll();
    if (ev.key == Keyboard::BACK) return;  // ESC always exits
    // handle other keys...
    delay(10);  // always 10ms at end of loop
}
```

### Adding a New Module

1. Create `src/modules/<name>/<name>.h`
2. Wrap everything in `namespace PascalName { ... } // namespace PascalName`
3. Expose `inline void run()` as the entry point
4. Add `#include "modules/<name>/<name>.h"` to `main.cpp`
5. Add a menu item: `menu.addItem("Label", "description", []() { PascalName::run(); });`

## Task Management

This project uses **Backlog.md** for task tracking. Interact via Backlog.md MCP tools
or the `backlog` CLI. Never edit task files in `backlog/` directly.
