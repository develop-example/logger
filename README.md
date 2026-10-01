# logger example

This project is the standalone example used to build the logger incrementally.

## Stage 8

Stage 8 adds configurable text and JSON formatting on top of the asynchronous
console/file Sink logger:

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
- file rolling by size with `B`, `KB`, `MB`, and `GB` units
- numbered file backups with a configurable retention count
- a record is never split across files; an oversized first record is kept intact
- thread-local `LogContext` fields with `set`, `get`, `erase`, `clear`, and `snapshot`
- nested `ScopedLogContext` with automatic full-snapshot restoration
- producer-side context capture for asynchronous records
- escaped context values in all Sink output
- `TextFormatter` with the default layout and configurable patterns
- `JsonFormatter` with stable fields and JSON escaping
- formatter and Sink configuration switched together on reload
- formatter failures counted separately in `QueueStats::format_errors`

Once/throttle macros, date-based rolling, custom external formatters, and
third-party backends are intentionally left for later stages.

The configuration format is intentionally small:

```properties
logger.level=INFO
logger.Motion.level=DEBUG
logger.Vision.level=WARN
logger.sinks=console,file
logger.file.path=logs/logger.log
logger.file.append=true
logger.file.max_size=10MB
logger.file.max_backups=5
logger.format=text
```

Set `logger.file.max_size=0` to disable rolling. When rolling is enabled,
`logger.file.max_backups` must be greater than zero; `logger.log.1` is the most
recent backup and older files receive larger suffixes.

Log context belongs to the calling thread and is copied into a record before
it enters the asynchronous queue:

```cpp
logger::ScopedLogContext context{
    {"request_id", "req-1001"},
    {"robot_id", "robot-01"}
};
LOGGER_INFO("start navigation");
```

Context fields are printed after the logger name, and values escape backslashes,
line breaks, tabs, carriage returns, and closing brackets. New threads start
with an empty context; use `LogContext::snapshot()` and `restore()` for
explicit propagation.

Set `logger.format=json` for one-line structured output. Text patterns support
`%datetime`, `%level`, `%thread`, `%name`, `%context`, `%file`, `%line`,
`%function`, `%message`, and `%%`. Unknown tokens are rejected during config
loading. JSON mode always uses its fixed schema and cannot be combined with
`logger.pattern`.

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
