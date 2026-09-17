# Solar Sail Simulator (Unreal Engine 5.7)

Solar Sail Simulator is a high-fidelity Unreal Engine 5.7 project focused on solar-sail dynamics under radiation pressure. It is a thesis/internship project by **Edoardo Conforti**, supervised by **Prof. Franco Milicchio**.

## Overview

The simulator models a sailcraft in a Sun-Earth scene using Unreal C++ actors, with configurable orbit initialization, force computation, and telemetry output for post-processing.

Core runtime classes:
- `ASolarSail` (`Simulation/Source/Simulation/Public/SolarSail.h`)
- `ASimulationManager` (`Simulation/Source/Simulation/Public/SimulationManager.h`)

## Implemented Features

- Unreal Engine **5.7** project (`Simulation/Simulation.uproject`)
- C++ simulation actors for sail and scenario management
- Configurable Earth gravity (`EnableGravity`) and solar radiation pressure (`EnableSolarPressure`)
- Solar-pressure force evaluation using sail orientation and reflectivity presets
- Multi-ray casting over a configurable grid (`GridResolution`) to sample illuminated sail area
- Eclipse/occlusion detection via ray intersections along sunward traces
- Selectable initial orbit presets and custom altitude:
  - `LEO_ISS`
  - `MEO_GPS`
  - `Geostationary`
  - `LunarDistance`
  - `CustomAltitude`
- Live telemetry/debug visualization in-engine (distances, normals, forces, photons, trails)
- CSV telemetry export in `Simulation/Csvs/` with header:
  - `SailName,Timestamp(s),DistEarth(Km),DistSun(Km),Velocity(Km/s),TotalForce(N),SolarPressure(Pa),IncidenceAngle(deg),ActivePhotons`
- Python analysis/plot scripts in `Analysis/` for telemetry validation and plotting
- LaTeX thesis sources in `Thesis/`

## Physics Model (Current Implementation)

The code computes Earth gravity acceleration and solar pressure from physical constants configured in `ASimulationManager`.

- Earth gravity magnitude:

  `a = (G * M_earth) / r^2`

- Solar radiation pressure:

  `P = L_sun / (4 * pi * r^2 * c)`

- Per-sample solar force contribution (as implemented in `ASolarSail::UpdateSolarForce`):

  `F_sample ∝ reflectivity * P * area_per_ray * cos(theta)^2`

Where `theta` is the incidence angle between sail normal and sun direction, and illumination is filtered by line-trace occlusion tests.

## Repository Structure

Only existing top-level directories are listed:

- `Simulation/` — Unreal Engine project, C++ source, maps, assets, CSV output folder
- `Analysis/` — Python scripts and generated plots/CSV copies for post-processing
- `Thesis/` — LaTeX thesis project and figures
- `Documents/` — reference papers and technical literature

Additional root files:
- [`DEVLOG.md`](DEVLOG.md)
- [`TODO.md`](TODO.md)

## Prerequisites

- Unreal Engine **5.7**
- A C++ toolchain compatible with Unreal on your platform
- Python 3 with:
  - `pandas`
  - `matplotlib`
  - `numpy`

## Basic Usage

### Run the Unreal simulation

1. Open `/home/runner/work/Solar-Sail-Simulator/Solar-Sail-Simulator/Simulation/Simulation.uproject` in Unreal Engine 5.7.
2. Build the project modules if prompted.
3. Load a simulation map from `Simulation/Content/` (for example `MainSimulation.umap`).
4. Configure `ASimulationManager` and `ASolarSail` actor parameters in the level.
5. Play the simulation; CSV telemetry is written to `Simulation/Csvs/` when logging is enabled.

### Run analysis scripts

From `/home/runner/work/Solar-Sail-Simulator/Solar-Sail-Simulator/Analysis`:

- `python SolarSailPlot.py`
- `python EclipsePlot.py`
- `python LambertPlot.py`
- `python EclipsePlot_Analitical.py`

Scripts read CSV telemetry and save plots under `Analysis/Plots/`.

## Current Status and Limitations

- Simulation logic is implemented in Unreal C++ (`ASolarSail`, `ASimulationManager`) and currently includes Earth gravity plus solar radiation pressure controls.
- Comments and many in-editor display labels in source code are currently in Italian.
- No repository license file is currently declared.

## License

No license is currently declared in this repository.
