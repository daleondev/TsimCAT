# TsimCAT - The Simulated Control and Automation Toolkit

## Project Overview
TsimCAT is a C++23 project designed to provide a unified, coroutine-based framework (**TLink**) for communicating with industrial automation protocols (ADS, OPC UA, TCP/UDP).

The core philosophy is to abstract the underlying driver mechanics (callbacks, threads, ASIO) behind a clean, linear, pull-based coroutine interface.

## Project Structure
*   **`tlink/`**: The header-only interface library.
    *   `include/tlink/core/`: Core coroutine types (`Task`, `Result`, `Context`, `AsyncChannel`).
    *   `include/tlink/driver.hpp`: The abstract `IDriver` interface that all protocols must implement.
*   **`external/`**: Third-party dependencies.
    *   `ADS/`: The Beckhoff ADS library (C++11, callback-based).
    *   `elements/`, `json/`, etc.: UI and utility libraries.
*   **`src/`**: Application source code.
    *   `main.cpp`: Currently contains a `MockDriver` and a demonstration of the TLink framework.

## Current Status (2025-12-28)

### Implemented
1.  **TLink Core Architecture:**
    *   `tlink::Task<T>`: A lazy-start coroutine return type (C++20/23).
    *   `tlink::Context`: A basic scheduler/event loop to resume suspended tasks.
    *   `tlink::Result<T>`: A standard `std::expected` wrapper for error handling.
    *   `tlink::AsyncChannel<T>`: A thread-safe, awaitable queue for subscriptions.
2.  **Abstract Driver Interface (`IDriver`):**
    *   `connect()` / `disconnect()`
    *   `read_raw()` / `write_raw()` (Request/Response)
    *   `subscribe()` (Returns a `DataStream` for pull-based updates)
3.  **Build System:**
    *   CMake project structure is modularized.
    *   `tlink` is an `INTERFACE` library.
    *   Fixed `ADS` library build issues (name collision with `semaphore.h`) by isolating include paths.

### Missing / To-Do
1.  **Real Drivers:**
    *   **ADS Driver:** Implement `IDriver` using `AdsLib`. Needs to bridge C-callbacks to `tlink::Task`.
    *   **TCP/UDP Driver:** Implement `IDriver` using `asio`.
2.  **Type System:** Currently `read_raw` returns `std::vector<std::byte>`. We need a serialization layer to read/write actual types (`int`, `float`, structs).
3.  **Endpoint Resolution:** The `IDriver` takes a `string_view path`. We need logic to resolve these paths to handles (e.g., ADS handles).
4.  **Application Logic:** The `main.cpp` is just a mock test. No actual simulation logic exists yet.

## Next Steps
1.  Implement the **ADS Driver** (`AdsDriver : public tlink::IDriver`).
2.  Test against a real TwinCAT PLC (or a local AMS router).
3.  Implement the **Typed Access Layer** (`tlink::Variable<T>`) to wrap the raw byte vectors.
