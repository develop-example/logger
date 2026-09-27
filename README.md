# logger example

This project is the standalone example used to build the logger incrementally.

## Stage 5

Stage 5 adds output Sink abstraction, console/file output, and multi-Sink
configuration on top of the asynchronous logger:

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
- bounded multi-producer queue with one output Worker
- configurable overflow policy and queue statistics
- `flush()` waits for queued and active records
- `shutdown()` stops producers, drains the queue, and joins the Worker
- `ConsoleSink` and `FileSink` behind an internal `ILogSink` interface
- simultaneous console and file output
- automatic file parent directory creation
- append or truncate file mode
- failed Sink writes are isolated and counted
- console and file sinks with a shared formatted record
- file sink directory creation and append/truncate mode
- sink failures are isolated and counted in `QueueStats::sink_errors`

Once/throttle macros, file rolling, and third-party backends are intentionally
left for later stages.

The configuration format is intentionally small:

```properties
logger.level=INFO
logger.Motion.level=DEBUG
logger.Vision.level=WARN
logger.sinks=console,file
logger.file.path=logs/logger.log
logger.file.append=true
```

Use `loadConfig(path)` to load a file explicitly. A failed load leaves the
currently active configuration unchanged. `reloadConfig()` reparses the last
successfully loaded path.

The default queue capacity is 8192 records and the default overflow policy is
`kDropLowPriority`: DEBUG and INFO records may be dropped when the queue is
full, while WARN, ERROR, and FATAL records wait for space. The policy and
capacity can be changed through `ILogger` before shutdown. `flush()` drains
the queue and then flushes the selected output stream; `shutdown()` is
idempotent and drains the queue before stopping the Worker.

## Build

```bash
cmake -S . -B build -DLOGGER_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/logger_example
```

No external logging library is required for this stage.
