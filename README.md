# TsimCAT

TsimCAT is a Qt 6.10 / C++23 digital twin for an industrial robot cell. The active target in this repository is a maintainable, simulation-first simple cell that can run either against real PLC-facing links or against an in-process local ADS shadow that behaves like PLC data without requiring TwinCAT at runtime.

## Active Cell Scope

The current implementation target is the simple cell:

- Input rotary table
- 6-axis robot
- Laser station
- Exit conveyor
- Guillotine dampers and safety fence

The code is being structured so the simple cell remains the working baseline while future stations can be added without reworking the composition root.

## Process Flow

The simple-cell process flow is:

1. A part is staged and indexed on the rotary input table.
2. The robot picks the part from the input side.
3. The part is transferred to the laser station.
4. The robot places the processed part onto the exit conveyor.
5. The conveyor and damper logic transfer the part out of the cell.

## Simulation Modes

TsimCAT is being set up to support two operating styles through the same station contracts:

- External link mode: station logic exchanges data through configured communication links.
- Local-only mode: station logic runs internally and publishes/consumes the same ADS-shaped symbols through an in-process ADS shadow.

The local ADS shadow is intended to make local simulation behave like PLC-driven operation instead of bypassing the communication model.

## Communication

All communication must be instantiated through `core::link::create()`.

Available link types in the project:

- ADS symbolic client
- TCP raw server
- OPC UA symbolic client
- In-process symbolic ADS shadow for local simulation

## Architecture

The project follows three main layers:

- `src/TsimCAT/Core`
	Pure logic only. No Qt headers. This layer owns links, simulators, kinematics, coordination logic, and logging.
- `src/TsimCAT/Backend`
	Qt-facing composition root. This layer owns the application backend, runtime config, and controller wrappers that expose Core state to QML.
- `src/TsimCAT/UI`
	QML and QtQuick3D presentation. This layer visualizes the cell and exposes operator/developer tooling for simulation.

Key rules:

- Backend is the composition root.
- Core owns plant behavior and local simulation logic.
- UI binds to backend controllers and should not implement plant sequencing.
- Real and local simulation paths should share the same symbol contracts where practical.

## Runtime Configuration

Runtime configuration is loaded from `config/runtime.json`.

The configuration is intended to cover:

- Link endpoints and network settings
- Station simulation modes
- ADS symbol mapping for robot and station data
- Simple-cell physical layout and dimensions
- Local simulation timing and behavior settings
- Logging and trace output

## Visual Target

The plant view is being upgraded from a placeholder arrangement into an explicit simple-cell layout with:

- A readable station-to-station flow
- A distinct rotary-table model instead of an entry placeholder
- Better material consistency across conveyors, fixtures, and fence elements
- Layout values that match the simulated components rather than one-off scene placements

## Build Requirements

- Windows 11
- MSVC (Visual Studio 2022 Build Tools, x64 host)
- vcpkg (`x64-windows`)
- CMake
- Ninja

Set `VCPKG_ROOT`:

```powershell
$env:VCPKG_ROOT = "C:/Dev/vcpkg"
```

Configure and build:

```powershell
cmake --preset debug
cmake --build --preset debug
```

Run:

```powershell
cmake --build --preset debug --target run_appTsimCAT
```

## Near-Term Implementation Direction

The next major work in this repository is focused on:

1. Cleaning up backend-owned flow logic and moving plant coordination into Core.
2. Adding a first-class rotary-table simulator and UI model.
3. Replacing hardcoded station/layout constants with runtime configuration.
4. Improving local simulation so ADS-like data exists even without TwinCAT.
5. Rebuilding the plant scene to match the actual simple-cell components.

## Status

TsimCAT is under active restructuring toward a cleaner simple-cell architecture. Some stations already simulate internal behavior, but the repository is being aligned so station visuals, runtime configuration, and local PLC-like data flow are all driven from the same design.

# Major Update: Finalizing debugging features, visuals and simulation
Goals:
1. Create advanced debugging features for AI agents to autonomously update the plant layout and station logic based on runtime observations. This means the AI needs to be able to take screenshots of the application, analyze the plant state, and make informed decisions about how to adjust the layout or station logic to optimize performance or address issues.
2. Finalize the plant visuals to accurately represent the simple cell, including the rotary table, robot, laser station and exit conveyor:
	- Add a Fence part above the rotary table to act as a safety barrier.
	- Update the rotatory table model to a contain a shield in its center as a safety feature.
	- adjust the fence height at the output conveyor to match the conveyor height.
	- Update the robot gripper to look more like a real gripper, and ensure it can interact with the parts on the rotary table and laser station in a visually realistic way.
	- Update the part visuals when gripped by the robot to look natural.
	- Remove the different colorings of the parts and make them all the same material, as they are all the same type of part in the simple cell.
	- Update the parts position in the laser station to be visible and adjust the laser beam in the laser station to point onto the part. (keep the general laser station layout the same)
3. Verify the correctness of the internal simulation logic for all stations and also prepare everything for control via external login through an actual PLC. This includes ensuring that the internal state of each station is accurately represented in the simulation and that the logic for processing parts through the stations is correct.