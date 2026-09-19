# logger example

This project is the standalone example used to build the logger incrementally.

## Stage 0

Stage 0 establishes the build and test skeleton only:

- C++17 static library target: `logger`
- public include directory: `include/logger/`
- executable smoke example: `logger_example`
- dependency-free test executable: `logger_test`
- install rules for the library and public headers

The library currently exposes only `logger::version()`. Actual logging,
macros, asynchronous processing, configuration, and backends are intentionally
left for later stages.

## Build

```bash
cmake -S . -B build -DLOGGER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/logger_example
```

No external logging library is required for this stage.
