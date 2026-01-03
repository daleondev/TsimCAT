# TsimCAT - The Simulated Control and Automation Toolkit

## Project Overview
TsimCAT is a C++23 project designed to provide a unified, coroutine-based framework (**TLink**) for communicating with industrial automation protocols (ADS, OPC UA, TCP/UDP).

The core philosophy is to abstract the underlying driver mechanics (callbacks, threads, ASIO) behind a clean, linear, pull-based coroutine interface.

## Project Structure
*   **`tlink/`**: The header-only interface library.
    *   `include/tlink/coroutine/`: Core coroutine types (`Task`, `Result`, `Context`, `Channel`).
    *   `include/tlink/driver.hpp`: The abstract `IDriver` interface that all protocols must implement.
    *   `drivers/`: Protocol-specific implementations.
        *   `ads/`: Beckhoff ADS driver (based on `AdsLib`).
        *   `opcua/`: OPC UA driver (based on `open62541`).
*   **`external/`**: Third-party dependencies.
    *   `ADS/`: The Beckhoff ADS library.
    *   `open62541/`: OPC UA implementation.
    *   `magic_enum`, `json`, etc.: Utility libraries.
*   **`src/`**: Application source code.
    *   `main.cpp`: Demonstration of the TLink framework (currently validating ADS Broadcast).

## Current Status (2025-12-30)

### Implemented
1.  **TLink Core Architecture:**
    *   **`tlink::coro::Task<T>`**: A lazy-start coroutine return type (C++20).
    *   **`tlink::coro::Context`**: A thread-safe scheduler/event loop.
    *   **`tlink::Result<T>`**: Error handling wrapper (`std::expected` equivalent).
    *   **`tlink::coro::BinaryChannel<T>`**: A robust, thread-safe, awaitable channel for byte-stream to type conversion.
    *   **`tlink::coro::Channel<T>`**: A generic, object-passing channel.
2.  **ADS Driver (`AdsDriver`):**
    *   **Functional:** Connect/Disconnect, Read/Write (span-based), and Subscribe.
    *   **Integration:** Wraps `AdsLib` callbacks into `BinaryChannel` streams.
    *   **Subscription:** Supports `OnChange` and `Cyclic` modes.
3.  **Drivers Infrastructure:**
    *   `IDriver` interface is stable.
    *   Factory/Registry pattern for managing driver instances and callbacks.

### In Progress / Partial
1.  **OPC UA Driver (`UaDriver`):**
    *   Structure exists (`tlink/drivers/opcua`).
    *   Links against `open62541`.
    *   **Status:** Stub implementation. Connection logic and data handling are not yet functional.

### Missing / To-Do
1.  **OPC UA Completion:** Implement the actual read/write/subscribe logic using `open62541`.
2.  **TCP/UDP Driver:** Implement `IDriver` using `asio`.
3.  **Advanced Type System:** Handling complex PLC structs automatically (reflection or code generation).
4.  **Simulation Runner:** Move beyond `main.cpp` to a configurable simulation loop.

---

## Detailed Coroutine Implementation

TLink relies heavily on C++20 coroutines to flatten asynchronous control flow. The implementation is located in `tlink/include/tlink/coroutine/`.

### 1. `Task<T>` (`Task.hpp`)
`Task<T>` is the standard return type for all asynchronous operations in TLink.
*   **Lazy Execution:** Tasks do not start immediately upon creation. They suspend at `initial_suspend` and only run when `co_await`ed.
*   **Promise Type:** Manages the result (`std::optional<T>`) or exception (`std::exception_ptr`).
*   **Awaitable:** Can be awaited by other coroutines. The `await_suspend` method stores the handle of the *waiting* coroutine to resume it when the task completes.
*   **Detached Tasks:** A helper `DetachedTask` and `co_spawn` exist for fire-and-forget scenarios where the task is submitted to an executor (`Context`) without being awaited.

### 2. `Context` (`Context.hpp`)
`Context` acts as the **Executor** and **Scheduler**. It provides the runtime environment for coroutines.
*   **Queue:** Maintains a thread-safe `std::deque<std::coroutine_handle<>>` of ready-to-run coroutines.
*   **Event Loop:** The `run()` method executes a loop that pops handles from the queue and calls `.resume()`. It uses a `std::condition_variable` to sleep when the queue is empty.
*   **Scheduling:** The `schedule(handle)` method pushes a handle onto the queue and notifies the loop. This is crucial for bridging asynchronous callbacks (like ADS notifications) back into the coroutine world.

### 3. `BinaryChannel<T>` & `Channel<T>` (`Channel.hpp`)
These are the communication backbones for subscriptions and event streams.

#### `BinaryChannel<T>`
Used by drivers to convert raw byte streams into typed values.
*   **`RawBinaryChannel`:** The type-erased implementation handling `std::vector<std::byte>`.
*   **Mechanism:** Uses `memcpy` to copy bytes into the target POD type `T`.

#### `Channel<T>`
Used for general-purpose object passing (e.g., Logging).
*   **Mechanism:** Moves full C++ objects (`T`) via a `std::deque<T>`.

#### Internal Structure (Shared)
*   **State Sharing:** Uses a `std::shared_ptr<State>` containing:
    *   `std::deque` (Bytes or T): Buffer for data.
    *   `std::deque<Waiter> waiters`: List of suspended coroutine handles.
    *   `std::mutex`: Protects all state access.

#### Operation Modes
The channels support two distinct modes (`ChannelMode`):
1.  **`Broadcast` (Pub/Sub):**
    *   **Default Behavior.**
    *   When data is `push`ed, **ALL** currently waiting coroutines receive a copy of the data.
    *   Waiters are immediately removed from the wait list and scheduled for resumption.
2.  **`LoadBalancer` (Work Queue):**
    *   When data is `push`ed, only **ONE** waiter (the one at the front of the queue) receives the data.

#### The `next()` Awaitable
Calling `co_await channel.next()` returns a `std::optional<T>`.
*   **Suspending:** If the channel is empty, the coroutine suspends.
*   **Resuming:** When a producer calls `push()`, the handle is retrieved and scheduled onto the `Context`.
*   **Closing:** If the channel is closed, `next()` returns `std::nullopt`.

### 4. Integration Example
When the ADS driver receives a notification callback (on a background thread):
1.  It locates the relevant `SubscriptionContext`.
2.  It calls `stream->push(data)`.
3.  The `BinaryChannel` locks its state.
4.  It identifies all waiting coroutines (consumers).
5.  It moves their handles to the `Context`'s queue via `executor->schedule()`.
6.  The `Context::run()` loop (on the main thread or worker thread) picks up the handle and resumes the coroutine.