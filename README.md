# TsimCAT - Industrial Control Center

TsimCAT is a modern industrial control and simulation interface built with **Qt 6.10.1**, **C++23**, **Asio**, and **QCoro**. It follows a strictly decoupled architecture separating high-performance C++ backend logic from a responsive, Material Design frontend.

## 🚀 Key Features
- **Modern UI**: 16:9 responsive layout with a persistent navigation sidebar using QtQuick Controls.
- **Async by Design**: 
    - **Core Logic**: Custom C++23 coroutine system (`core::coro`) for high-performance, non-blocking operations.
    - **UI Glue**: Integration with **QCoro** for seamless Qt event loop interoperability.
- **Multi-Protocol Connectivity**:
    - **ADS**: Native integration with Beckhoff TwinCAT via `AdsLib`.
    - **OPC UA**: Client support via `open62541`.
    - **Raw TCP**: Asynchronous TCP server/client implementation using `Asio`.
- **Procedural Graphics**: Icons and visuals rendered using QML Shapes (asset-free).
- **Clean Architecture**: 
    - **Core**: Pure C++23 header-only/static libraries (Coroutines, Links, Simulators). independent of Qt.
    - **Backend**: Qt-based "Glue" layer bridging Core logic to QML.
    - **UI**: Modular QML components.

## 📁 Project Structure
```text
TsimCAT/
├── extern/               # Third-party dependencies (Git Submodules)
│   ├── ADS/              # Beckhoff ADS library
│   ├── asio/             # Asynchronous I/O (Networking)
│   ├── open62541/        # OPC UA implementation
│   └── qcoro/            # Coroutine support for Qt
├── src/
│   ├── app/              # Application entry point (main.cpp)
│   └── TsimCAT/
│       ├── Core/         # [Logic] Pure C++23 Domain Layer
│       │   ├── Common/   # Shared types (Result, etc.)
│       │   ├── Coroutines/ # Custom Task/Context system
│       │   ├── Link/     # Communication Drivers (ADS, Tcp, OpcUa)
│       │   └── Simulators/ # Simulation logic
│       ├── Backend/      # [Glue] Qt/C++ Bridge (QObject, Q_PROPERTY)
│       └── UI/           # [View] QML Frontend Module (TsimCAT.UI)
│           ├── controls/ # Custom reusable QML controls
│           ├── icons/    # Procedural icon components
│           └── subscreens/ # Main application views
├── .vscode/              # IDE configuration for IntelliSense
├── CMakeLists.txt        # Root build configuration
└── CMakePresets.json     # Build presets (Debug/Release)
```

## 🛠 Prerequisites
- **Qt 6.10.1** (MinGW 64-bit)
- **CMake 3.16+**
- **Ninja** (Build generator)
- **MinGW GCC 15.2+** (C++23 support)

## 🔨 Building and Running

### Command Line
1. **Initialize Submodules** (if fresh clone):
   ```powershell
   git submodule update --init --recursive
   ```

2. **Configure**:
   ```powershell
   cmake --preset debug
   ```

3. **Build**:
   ```powershell
   cmake --build --preset debug
   ```

4. **Run**:
   ```powershell
   # Ensure Qt binaries are in PATH
   $env:PATH = "D:\Qt\6.10.1\mingw_64\bin;" + $env:PATH
   .\build\debug\appTsimCAT.exe
   ```

### VS Code Integration
The project is pre-configured for VS Code:
- **IntelliSense**: Powered by `compile_commands.json` (auto-generated).
- **Extension**: Recommended to use "CMake Tools" and "C/C++" extension.

## 💡 Development Patterns

### Core Logic (Pure C++)
Implement business logic in `src/TsimCAT/Core`. Use `core::coro::Task` for async operations.
```cpp
// Example: src/TsimCAT/Core/Link/Raw/TcpServer.cpp
auto TcpServer::accept(std::chrono::milliseconds timeout) -> coro::Task<result::Result<void>>
{
    // Implementation using Asio bridge
    co_await m_acceptor.async_accept(m_socket, asio::use_awaitable);
    co_return result::success();
}
```

### Bridging Logic to UI
Always follow the "Glue" pattern:
1. Implement heavy logic in `Core/`.
2. Expose the logic to QML in `Backend/Backend.h` using `Q_PROPERTY` and `Q_INVOKABLE`.
3. Use `QCoro::Task` to await Core tasks if necessary, or manage them via the Core Context.

## 📌 Current Status
- [x] Hierarchical folder structure refactored (`Core`, `Backend`, `UI`).
- [x] **Core**: Custom Coroutine system implemented.
- [x] **Core**: Asio-based TCP Server with coroutine bridge (`AsioAwaiter`).
- [x] **Core**: ADS Link and OPC UA Client skeletons/implementations.
- [x] **UI**: Navigation sidebar with 6 subscreens.
- [x] **UI**: Procedural icons for all subsystems.
- [x] Build system fully configured with CMake Presets.