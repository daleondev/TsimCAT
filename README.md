# TsimCAT - Industrial Control Center

TsimCAT is a modern industrial control and simulation interface built with **Qt 6.10.1**, **C++23**, and **QCoro**. It follows a strictly decoupled architecture separating high-performance C++ backend logic from a responsive, Material Design frontend.

## 🚀 Key Features
- **Modern UI**: 16:9 responsive layout with a persistent navigation sidebar.
- **Async by Design**: Full integration of C++23 coroutines via QCoro for non-blocking I/O and UI.
- **Procedural Graphics**: Icons are rendered using QML Shapes (no external image assets required).
- **Clean Architecture**: 
    - **Logic**: Pure C++23 header-only libraries (Network, Coroutine).
    - **Glue**: Qt-based Backend classes bridging Logic to QML.
    - **UI**: Modular QML components organized by responsibility.

## 📁 Project Structure
```text
TsimCAT/
├── extern/               # Third-party dependencies (Git Submodules)
│   └── qcoro/            # Coroutine support for Qt
├── src/
│   ├── app/              # Application entry point (main.cpp)
│   └── TsimCAT/
│       ├── Backend/      # C++ Backend & Glue Logic
│       │   ├── coroutine/ # [Logic] Pure C++ Coroutine Task system
│       │   └── network/   # [Logic] Pure C++ Network Driver abstractions
│       └── UI/           # QML Frontend Module (TsimCAT.UI)
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
   $env:PATH = "D:\Qt\6.10.1\mingw_64\bin;" + $env:PATH
   .\build\debug\appTsimCAT.exe
   ```

### VS Code Integration
The project is pre-configured for VS Code:
- **IntelliSense**: Powered by `compile_commands.json` (auto-generated).
- **Styles**: Defaulted to Material Light via `qtquickcontrols2.conf`.
- **Extension**: Recommended to use "CMake Tools" and "C/C++" extension.

## 💡 Development Patterns

### Adding a New Icon
1. Create `NewIcon.qml` in `src/TsimCAT/UI/icons/`.
2. Define the icon using `Shape` and `ShapePath`.
3. Add the file to `src/TsimCAT/UI/CMakeLists.txt`.
4. Use it in the sidebar model in `Main.qml`.

### Bridging Logic to UI
Always follow the "Glue" pattern:
1. Implement heavy logic in `Backend/network/` or `Backend/coroutine/` as pure C++.
2. Expose the logic to QML in `Backend.h` using `Q_PROPERTY` and `Q_INVOKABLE`.
3. Use `QCoro::Task` to run logic asynchronously:
   ```cpp
   QCoro::Task<void> Backend::runAsyncLogic() {
       auto result = co_await m_driver.getData();
       this->setStatus(result);
   }
   ```

## 📌 Current Status
- [x] Hierarchical folder structure.
- [x] QCoro integration & verification test.
- [x] Navigation sidebar with 6 subscreens.
- [x] Procedural icons for all subsystems.
- [x] VS Code IntelliSense fixed.
