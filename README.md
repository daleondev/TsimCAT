# TsimCAT - Industrial Control Center

TsimCAT is a modern industrial control and simulation interface built with **Qt 6.10.1**, **C++23**, **Asio**, and **QCoro**. It follows a strictly decoupled architecture, hiding complex driver implementations behind clean interfaces and a centralized factory.

## 🚀 Key Features
- **Modern UI**: Material Design 16:9 responsive layout with procedural graphics.
- **3D Digital Twin**: Real-time robot visualization using **QtQuick3D**.
    - Integrated 6-axis kinematic chain with mesh-based rendering.
    - Custom CAD-style camera controls (Right-click Orbit, Left-click Pan, Scroll Zoom).
- **Advanced Architecture**: 
    - **Composition Root**: A thin `Backend` wrapper managing specialized domain `Controllers`.
    - **Interface-Driven Drivers**: Drivers (ADS, TCP, OPC UA) are hidden behind a `LinkFactory`. No driver-specific headers (Asio, AdsLib) leak into the rest of the project.
- **Sophisticated Logging**:
    - Thread-safe `core::logger` module with console (ANSI colors) and file output.
    - Automated directory structure (`logs/`).
    - Uses `format_utils` for type-safe, compile-time/runtime formatting and `std::source_location` for automatic call-site tracking.
- **Diagnostics**:
    - Real-time TCP server status and message logging.
    - Integrated UI screenshot capability for debugging purposes.

## 📁 Project Structure
```text
TsimCAT/
├── logs/                 # Auto-generated log files
├── screenshots/          # Debugging captures
├── src/
│   ├── app/              # Entry point (main.cpp)
│   └── TsimCAT/
│       ├── Core/         # [Logic] Pure C++23 Domain Layer (Qt-Independent)
│       │   ├── Common/   # Shared types (Result, etc.)
│       │   ├── Coroutines/ # Custom Task system
│       │   ├── Logger/   # Advanced logging framework
│       │   ├── Link/     # Communication abstraction
│       │   └── Simulators/ # Domain simulation (Robot, Laser)
│       ├── Backend/      # [Glue] Qt/C++ Bridge
│       │   ├── Backend.h # Composition Root
│       │   └── Controllers/ # Feature-specific logic (e.g., RobotController)
│       └── UI/           # [View] QML Frontend
│           ├── assets/   # 3D Meshes and icons
│           ├── controls/ # Reusable UI components (Robot3DView)
│           └── subscreens/ # Full-page views
```

## 🛠 Prerequisites & Build
- **Qt 6.10.1** (MinGW 64-bit) with **QtQuick3D** module
- **MinGW GCC 15.2+** (Required for C++23 features like `std::print`)
- **CMake 3.24+** & **Ninja**

### Building
```powershell
cmake --preset debug
cmake --build --preset debug
```

### ⚠️ Running (Critical DLL Order)
To avoid "Entry Point Not Found" errors, the **Compiler's bin directory must be prioritized** over Qt's bin directory in the PATH to ensure the correct `libstdc++` version is loaded:
```powershell
$env:PATH = "C:\Users\Dev.Windows-Desktop\AppData\Local\mingw64\bin;D:\Qt\6.10.1\mingw_64\bin;" + $env:PATH
.\build\debug\appTsimCAT.exe
```

## 💡 Architectural Patterns

### The Link Factory
Drivers are instantiated via `core::link::create()`. This returns a `result::Result<std::unique_ptr<ILink>>`.
Users work with capabilities via safe casting:
```cpp
auto link = core::link::create(Role::Server, Mode::Raw, Protocol::Tcp, config);
if (auto* server = link->asServer()) {
    server->start();
}
```

### 3D Kinematics
The 3D robot visualization uses a nested `Node` structure to represent a standard 6-axis kinematic chain. The `Robot3DView.qml` component maps joint angles directly to these nodes, allowing the UI to reflect the simulated robot state in real-time.

## 📌 Development Status
- [x] **3D Digital Twin**: Implemented 6-axis robot visualization with custom camera navigation.
- [x] **Namespace Cleanup**: Standardized on `core::` and `backend::`.
- [x] **Encapsulation**: Driver dependencies (`asio`, `ads`, `open62541`) are now **PRIVATE** to the link module.
- [x] **TCP Robustness**: Fixed socket reuse, disconnect detection (EOF handling), and `NO_TIMEOUT` logic.
- [x] **Logging**: Implemented folder-based logging with source tracking.
- [x] **Screenshots**: UI capture logic implemented.