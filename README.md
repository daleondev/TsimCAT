# TsimCAT - Simple Cell

TsimCAT is a Windows/MSVC Qt 6.10.1 digital twin focused on a reduced industrial cell.

## Scope (this branch)
- Robot (6-axis)
- Conveyor 1 (Entry)
- Conveyor 2 (Exit)
- Safety fence
- Two guillotine dampers

Removed from this branch:
- Camera station
- Laser station
- Gantry
- Transfer conveyor (Conveyor 3)
- Related UI pages, controllers, simulators, and assets

## Drivers
All communication is still instantiated via `core::link::create()`.

Kept and build-enabled for future use:
- ADS (active in current simulation)
- TCP Raw server (configured/available)
- OPC UA client (configured/available)

## Build Requirements
- Windows 11
- MSVC (Visual Studio 2022 Build Tools, x64 host)
- vcpkg (`x64-windows`)
- CMake + Ninja

Set `VCPKG_ROOT`:
```powershell
$env:VCPKG_ROOT = "C:/Dev/vcpkg"
```

Build:
```powershell
cmake --preset debug
cmake --build --preset debug
```

Run:
```powershell
cmake --build --preset debug --target run_appTsimCAT
```

## Runtime Configuration
Runtime settings are loaded from `config/runtime.json`.

Key active sections:
- `links.tcp`
- `links.ads`
- `links.opcUa`
- `simulation.stationModes.robotInternal`
- `simulation.stationModes.entryConveyorInternal`
- `simulation.stationModes.exitConveyorInternal`
- `adsVariables.robot`
- `adsVariables.conveyors.entry*`
- `adsVariables.conveyors.exit*`

## Architecture
- `src/TsimCAT/Core`: pure logic, no Qt headers
- `src/TsimCAT/Backend`: composition root and Qt bridge
- `src/TsimCAT/UI`: QML visualization (Plant Overview + Robot Status)

Project is configured as Windows + MSVC only.
