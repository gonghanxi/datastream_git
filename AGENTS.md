# AGENTS.md — GWDataFlowSimulator

## Project Overview

Qt/C++ radar signal flow simulation platform. Builds a main executable (`SignalFlowSimulator.exe`) that dynamically loads a core library (`SignalFlowSimulatorLib.dll`) and ~280 signal processing model plugins (each a separate DLL). Supports FMU co-simulation, DDS communication, and WebSocket control.

## Build System

**qmake** (Qt 5, C++17). No CMake at the project level (CMakeLists.txt only inside vendored dependencies like Eigen).

### Build Order (required)

Dependencies must be built in this order:

1. `ModelDesign/ModelDesign.pro` → produces static lib `DataStream.lib` into `ModelDesign/lib/`
2. `SignalFlowSimulatorLib/SignalFlowSimulatorLib.pro` → produces `SignalFlowSimulatorLib.dll` into `bin/`
3. `Simulator/Simulator.pro` → produces `SignalFlowSimulator.exe` into `bin/`
4. `model_project/model_project.pro` → produces ~280 model DLLs into `bin/models/`

### Build Commands (Windows/MSVC)

```bat
qmake ModelDesign\ModelDesign.pro -o build_ModelDesign\Makefile
jom -f build_ModelDesign\Makefile release

qmake SignalFlowSimulatorLib\SignalFlowSimulatorLib.pro -o build_dataStream\Makefile
jom -f build_dataStream\Makefile release

qmake Simulator\Simulator.pro -o build_Simulator\Makefile
jom -f build_Simulator\Makefile release

qmake model_project\model_project.pro -o build_Models\Makefile
jom -f build_Models\Makefile release
```

Or use Qt Creator to open the `.pro` files and build from the IDE.

### Linux Build

```bash
qmake ModelDesign/ModelDesign.pro -o build_ModelDesign/Makefile
make -C build_ModelDesign release

qmake SignalFlowSimulatorLib/SignalFlowSimulatorLib.pro -o build_dataStream/Makefile
make -C build_dataStream release

qmake Simulator/Simulator.pro -o build_Simulator/Makefile
make -C build_Simulator release

qmake model_project/model_project.pro -o build_Models/Makefile
make -C build_Models release
```

Linux produces `.so` files instead of `.dll`. The executable tries `libSignalFlowSimulatorLib.so` first, then `SignalFlowSimulatorLib.so`.

### Separate Builds (not part of main sequence)

| Component | Build System | Notes |
|---|---|---|
| `dds端代码/Simulation/` | Visual Studio (`.sln`) | Windows-only. Links `DispersedAgent.lib` + Windows system libs. Outputs to `x64/Release/`. |
| `FmuExportEngine/src/` | qmake | Standalone FMU packaging tool. C++11. Not linked to main sim. |
| `WebSocket/` | qmake | Standalone WebSocket client for testing. |
| `VirtualRManager/` | qmake | Standalone instrument manager (serial/TCP/UDP/USB). |
| `radar_servo_system/` | qmake (static lib) | Produces `radar_servo_system.lib` into `lib/`. Required by `udpSource` plugin but not included in main build. |

## Key Directories

| Directory | Purpose |
|---|---|
| `ModelDesign/` | Core data flow framework: Block, Buffer, Port, DFModel, SimulationControl. Static lib `DataStream.lib`. |
| `SignalFlowSimulatorLib/` | Main simulation engine: SimRunner, schedulers (data-flow & time-driven), connection validation, FMU support, DDS controller, variable analysis. |
| `Simulator/` | CLI entrypoint. Loads `SignalFlowSimulatorLib.dll` dynamically via `QLibrary`, calls `createSimRunner()` (C extern). |
| `model_project/` | ~280 signal processing model plugins. Each subdirectory is a standalone DLL plugin (Add, FFT_Cx, RADAR_*, etc.). |
| `FMUManager/` | FMU (FMI 2.0) co-simulation support. Included via `.pri` into SignalFlowSimulatorLib. |
| `FmuExportEngine/` | Standalone tool to export simulations as FMU packages. |
| `Common/` | Shared interfaces: `ISimRunner`, `ILogWriter`, `SimCfgData`. |
| `WebSocket/` | WebSocket client for remote control. |
| `VirtualRManager/` | Virtual instrument manager (serial, TCP, UDP, USB connections). |
| `udpSink/`, `udpSource/` | UDP communication model blocks. Live at repo root; **not** included in `model_project` SUBDIRS. `udpSource` depends on `radar_servo_system`. |
| `radar_servo_system/` | Radar servo control system (host communication, signal processing). Static lib. |
| `dds端代码/` | DDS (Data Distribution Service) simulation component. Visual Studio solution (`Simulation.sln`). Windows-only. |
| `bin/` | Build output directory. Contains the executable, DLLs, Qt runtime, and model DLLs. **Not tracked by git** (`.gitignore` excludes it). |

## Architecture Notes

- **Plugin architecture**: `Simulator.exe` loads `SignalFlowSimulatorLib.dll` via `QLibrary` and calls `createSimRunner()` (C extern function). The lib then loads model DLLs from `bin/models/`.
- **Two scheduler modes**: `SimpleScheduler` (data-flow driven) and `TimeDrivenScheduler` (time-step driven). Selected per simulation configuration.
- **Link files**: JSON files describing the signal flow graph (blocks, connections, parameters). Passed as `[file1.json,file2.json]` format to the executable. The brackets are mandatory — the parser rejects paths without `[`/`]`.
- **Nested sub-systems**: Blocks can contain sub-link files, parsed recursively via `recursiveReadBlock()`.
- **Port validation**: `ConnectionValidator` and `PortValidatorImpl` enforce type compatibility between connected ports.
- **Topology sort**: `SignalFlowLinkSort` determines block execution order.
- **Short/open circuit detection**: `ShortOpenProcessor` validates signal source/sink connectivity.
- **DDS controller**: `SimEngineController` in `SignalFlowSimulatorLib/DDS/` provides DDS-based remote simulation control.

## External Dependencies

- **Qt 5**: Core, Network, WebSockets, Concurrent, SerialPort, OpenGL, SVG, PrintSupport
- **OpenBLAS**: Linear algebra. Windows: prebuilt in `ModelDesign/openBlas/lib/x64_release/`. Linux: system `libopenblas`.
- **FFTW 3.3.5**: Used by `RADAR_MTD` model. Prebuilt in `model_project/RADAR_MTD/fftw-3.3.5/`.
- **Eigen 3.4.1**: Used by `RADAR_DOA` model. Vendored in `model_project/RADAR_DOA/eigen-3.4.1/`.
- **nlohmann/json**: Single-header JSON library at `ModelDesign/json.hpp`.

## Platform Quirks

- **Windows encoding**: MSVC `/utf-8` flag is set. Console uses `SetConsoleOutputCP(CP_UTF8)`. Input args decoded as GBK on Windows, UTF-8 on Linux.
- **Linux visibility**: `-fvisibility=hidden -fvisibility-inlines-hidden` for model plugins. Only `RegisterModel`-exported symbols are visible.
- **`QT_NO_DEBUG_OUTPUT`**: Defined on Linux release builds for ModelDesign, SignalFlowSimulatorLib, and model plugins. Suppresses `qDebug()` output.
- **Runtime timeout**: Simulation has a hard 10-minute timeout enforced in `Simulator/main.cpp` via `QTimer`. No way to override from link files.
- **Stdin control**: The executable listens on stdin for `pause`/`continue`/`stop` commands via `StdinListener` thread.
- **Executable args**: Exactly 3 args required: `linkFiles outputPath userId`. `linkFiles` must be `[path1,path2]` format.
- **Linux RPATH**: Simulator sets `$$ORIGIN` as RPATH so it finds `.so` files next to the executable.
- **Model .so naming**: On Linux, model plugins use `CONFIG += unversioned_libname` and `QMAKE_EXTENSION_SHLIB = so` to produce clean filenames like `Add.so` (not `libAdd.so.1.0.0`).

## Adding a New Model Block

1. Create a new directory under `model_project/YourBlock/`.
2. Create `YourBlock.pro` following existing patterns (TEMPLATE = lib, CONFIG += dll, link against DataStream.lib from `ModelDesign/lib/`).
3. Create `YourBlock.pri` for header/source inclusion.
4. Add the block to `model_project/model_project.pro` SUBDIRS list.
5. Implement the block interface: inherit from `Block` (in `ModelDesign/Block.h`), register via `RegisterModel`.

## Testing

No automated test framework. Verification is done by running simulations with specific link files and checking output:

```bat
bin\SignalFlowSimulator.exe "[path\to\link.json]" path\to\output\ userId
```

## Vendored Dependencies (do not modify)

- `model_project/RADAR_DOA/eigen-3.4.1/` — Eigen library
- `model_project/RADAR_MTD/fftw-3.3.5/` — FFTW library
- `ModelDesign/openBlas/` — OpenBLAS prebuilt binaries and wrappers

## Gotchas

- **Merge conflict debris**: `SignalFlowSimulatorLib/simrunner.h` has `.BASE.h`, `.LOCAL.h`, `.REMOTE.h` variants — leftover merge artifacts, not used in the build.
- **`.gitignore` excludes `bin/`**: The `bin/` directory with pre-built outputs is not tracked. After a fresh clone you must build everything before running.
- **`udpSink`/`udpSource` are NOT in `model_project` SUBDIRS**: They live at the repo root and have their own `.pro` files. They are not built by the standard build sequence. `udpSource` also depends on `radar_servo_system.lib`.
- **DDS is Windows-only**: The `dds端代码/` component uses Visual Studio and Windows system libraries. It cannot be built on Linux.
- **`DataStream.lib` not tracked**: `.gitignore` excludes `ModelDesign/lib/DataStream.lib`. You must build ModelDesign first before anything else will link.
