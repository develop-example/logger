# logger example

This project is the standalone example used to build the logger incrementally.

## Stage 3

Stage 3 adds properties-based configuration and module-level filtering on top
of the synchronous, dependency-free logger and public macros:

- C++17 static library target: `logger`
- public include directory: `include/logger/`
- executable smoke example: `logger_example`
- dependency-free test executable: `logger_test`
- install rules for the library and public headers
- five log levels with a global threshold
- `printf` and `stringstream` logging interfaces
- structured `LogRecord` metadata
- thread-safe console output through `std::clog`
- configurable output stream for embedding and tests
- `LOGGER_DEBUG/INFO/WARN/ERROR/FATAL` macros
- named and stream-style macro variants
- generated `MODULE_LOG_*` wrappers for Motion and Vision
- source file, line, and function metadata captured automatically by macros
- filtered stream expressions are not evaluated
- root and module-specific log levels from `config/logger.properties`
- nearest-parent module level resolution
- runtime configuration loading and reloading
- `LOGGER_CONFIG_FILE` environment variable support

Once/throttle macros, asynchronous processing, file output, and third-party
backends are intentionally left for later stages.

The configuration format is intentionally small:

```properties
logger.level=INFO
logger.Motion.level=DEBUG
logger.Vision.level=WARN
```

Use `loadConfig(path)` to load a file explicitly. A failed load leaves the
currently active configuration unchanged. `reloadConfig()` reparses the last
successfully loaded path.

## Build

```bash
cmake -S . -B build -DLOGGER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/logger_example
```

No external logging library is required for this stage.
