# TsimCAT - Industrial Control Center

TsimCAT is a modern industrial control and simulation interface built with **Qt 6.10.1**, **C++23**, **Asio**, and **QCoro**. It follows a strictly decoupled architecture, hiding complex driver implementations behind clean interfaces and a centralized factory. The application features a high-fidelity **3D Digital Twin** of an industrial manufacturing cell, built entirely with **QtQuick3D**.

## 🚀 Key Features
- **Modern UI**: Material Design 16:9 responsive layout with procedural graphics.
- **3D Digital Twin**: High-performance, real-time visualization of a complete industrial plant.
    - **6-Axis Robot**: Kinematically accurate simulation with real-time joint feedback.
    - **Interactive Plant**: Includes conveyors, gantry systems, and processing stations.
    - **Safety Systems**: Visualized safety fencing with interactive doors and dampers.
- **Advanced Architecture**: 
    - **Composition Root**: A thin `Backend` wrapper managing specialized domain `Controllers`.
    - **Interface-Driven Drivers**: Drivers (ADS, TCP, OPC UA) are hidden behind a `LinkFactory`.
- **Sophisticated Logging**:
    - Thread-safe `core::logger` module with console (ANSI colors) and file output.
- **Diagnostics**:
    - Real-time connectivity status and integrated UI screenshot capability.

## 🎨 UI & 3D Visualization

The user interface is powered by **QtQuick 3D**, moving beyond static dashboards to provide a fully immersive operational view.

### 🏭 Plant Overview (`Plant3DView`)
A comprehensive digital twin of the manufacturing cell, featuring:
*   **Conveyor System**: A dual-belt setup simulating material flow.
    *   **Entry Conveyor**: Transports parts into the cell, passing through a safety-interlocked **Guillotine Damper**.
    *   **Exit Conveyor**: Moves processed parts out of the cell to a higher-level neighbor belt.
*   **Processing Stations**:
    *   **Analysis Station**: Features a frame-mounted camera with a visualized **Field of View (FOV)** cone.
    *   **Laser Station**: Equipped with a 30° tilted laser head and visible beam for part marking.
*   **Transfer Gantry**: A 2-axis (Y/Z) linear robot with a detailed **Industrial Gripper**, bridging the cell's exit to the neighbor's logistics.
*   **Safety Perimeter**: A modular **Chain-Link Fence** system (procedurally textured) ensuring operator safety, complete with:
    *   **Double Safety Door**: Animated swinging doors with "soft-stop" mechanics.
    *   **Safety Interlocks**: Visual feedback for open/closed states.

### 🤖 Robot Control (`Robot3DView`)
A focused, detailed view of the 6-axis industrial manipulator.
*   **Kinematics**: Joint angles are bound directly to the C++ backend simulation.
*   **Visualization**: High-fidelity mesh rendering with PBR materials (PrincipledMaterial) for realistic metal and paint finishes.
*   **Navigation**: CAD-style camera controls (Orbit, Pan, Zoom) allow operators to inspect the robot from any angle.

### 🛠 reusable 3D Components
The 3D scene is built from modular, reusable QML components found in `src/TsimCAT/UI/controls/`:
*   `RobotModel.qml`: The core 6-axis kinematic chain.
*   `GantryModel.qml`: Parametric 2-axis gantry with Z-rod and gripper.
*   `FenceModel.qml`: Procedurally generated wire-mesh fence with door/damper logic.
*   `CameraModel.qml` / `LaserModel.qml`: Sensor and tool heads with mounting frames.

## 📁 Project Structure
```text
TsimCAT/
├── logs/                 # Auto-generated log files
├── screenshots/          # Debugging captures
├── src/
│   ├── app/              # Entry point (main.cpp)
│   └── TsimCAT/
│       ├── Core/         # [Logic] Pure C++23 Domain Layer (Qt-Independent)
│       │   ├── Link/     # Communication abstraction (ADS, TCP)
│       │   └── Simulators/ # Domain simulation logic (Robot kinematics)
│       ├── Backend/      # [Glue] Qt/C++ Bridge
│       │   ├── Backend.h # Composition Root
│       │   └── Controllers/ # Feature-specific logic (e.g., RobotController)
│       └── UI/           # [View] QML Frontend
│           ├── assets/   # 3D Meshes (.mesh)
│           ├── controls/ # 3D Components (RobotModel, FenceModel, etc.)
│           └── subscreens/ # Full-page views (PlantOverview, RobotStatus)
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

### Runtime Configuration
Runtime endpoints and simulator parameters are loaded from `config/runtime.json`.
Per-station local simulation can be configured independently via:
- `simulation.stationModes.robotInternal`
- `simulation.stationModes.laserInternal`
- `simulation.stationModes.entryConveyorInternal`
- `simulation.stationModes.exitConveyorInternal`

Analyzer capture can be configured via `analyzer`:
- `enabled`: Enable/disable analyzer helper pipeline.
- `autoStart`: Start capture automatically on app launch.
- `saveFrames`: Write cyclic PNG frames.
- `saveTrace`: Write CSV state trace (`trace.csv`).
- `frameIntervalMs` / `traceIntervalMs`: Capture cadence.
- `maxFrames`: Automatic stop after N frames.
- `outputFolder`: Output directory for frames and trace.

Trace diagnostics can be configured via `trace`:
- `enabled`: Enable structured protocol/state/flow/invariant JSONL output.
- `outputFolder` + `fileName`: Target trace file (default `analysis/session/protocol_trace.jsonl`).
- `sampleIntervalMs`: Optional event throttling in milliseconds.
- `stationFilter`: Optional station whitelist.

Runtime log behavior:
- `logs/TsimCAT.log` is reset on each application start, so it always contains only the current run.

`simulation.localOnly=true` forces all stations to local mode; keep it `false` to control each station individually.

You can override the file path with:
```powershell
$env:TSIMCAT_CONFIG = "config/runtime.json"
```

### Trace Summary Tool

Use the helper script to summarize a single run from JSONL trace data:

```powershell
pwsh .\analysis\tools\summarize-trace.ps1
```

Optional JSON output:

```powershell
pwsh .\analysis\tools\summarize-trace.ps1 -AsJson
```

### ⚠️ Running (Critical DLL Order)
To avoid "Entry Point Not Found" errors, the **Compiler's bin directory must be prioritized** over Qt's bin directory in the PATH:
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
The robot visualization uses a nested `Node` structure to represent a standard 6-axis kinematic chain. The `RobotModel.qml` component maps joint angles from the C++ `RobotController` directly to these nodes, enabling real-time, jitter-free visualization of the machine state.

## 📌 Development Status
- [x] **3D Digital Twin**: Full plant layout with Robot, Gantry, Conveyors, and Stations.
- [x] **Safety Systems**: Interactive fencing, doors, and dampers.
- [x] **Namespace Cleanup**: Standardized on `core::` and `backend::`.
- [x] **Encapsulation**: Driver dependencies (`asio`, `ads`, `open62541`) are **PRIVATE**.
- [x] **TCP Robustness**: Fixed socket reuse and disconnect detection.
- [x] **Logging**: Implemented folder-based logging with source tracking.
- [x] **Screenshots**: Integrated UI capture logic.
