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

## Current Status (2025-12-29)

### Implemented
1.  **TLink Core Architecture:**
    *   `tlink::Task<T>`: A lazy-start coroutine return type (C++20/23).
    *   `tlink::Context`: A basic scheduler/event loop to resume suspended tasks.
    *   `tlink::Result<T>`: A standard `std::expected` wrapper for error handling.
    *   `tlink::AsyncChannel<T>`: A thread-safe, awaitable queue for subscriptions.
2.  **Abstract Driver Interface (`IDriver`):**
    *   `connect()` / `disconnect()`
    *   `readInto()` / `writeFrom()` (Raw span-based access)
    *   `read<T>()` / `write<T>()`: Basic typed access using `std::span`.
    *   `subscribe()`: Interface for pull-based data streams.
3.  **ADS Driver (Partial):**
    *   Basic connection management and handle-based Read/Write.
    *   Integration with `AdsLib` (Beckhoff Open Source).
4.  **Build System:**
    *   CMake project structure is modularized.
    *   `tlink` is an `INTERFACE` library.

### Missing / To-Do
1.  **ADS Driver Completion:**
    *   Finish `subscribe()` / `unsubscribe()` logic.
    *   Optimize handle management (caching handles instead of re-fetching).
    *   Properly bridge ADS callbacks to `tlink::AsyncChannel`.
2.  **TCP/UDP Driver:** Implement `IDriver` using `asio`.
3.  **Advanced Type System:** Beyond simple PODs, we need a way to handle PLC-specific strings and complex structs automatically.
4.  **Application Logic:** Move beyond `main.cpp` demos to a proper simulation runner.

## Next Steps
1.  Implement the **ADS Driver** (`AdsDriver : public tlink::IDriver`).
2.  Test against a real TwinCAT PLC (or a local AMS router).
3.  Implement the **Typed Access Layer** (`tlink::Variable<T>`) to wrap the raw byte vectors.
