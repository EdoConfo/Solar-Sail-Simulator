# Solar Sail Simulator (UE5)

High-fidelity solar-sail simulation developed as an internship and thesis project in Computer Engineering.

**Author:** Edoardo Conforti  
**Supervisor:** Prof. Franco Milicchio

## Overview

Solar Sail Simulator is an Unreal Engine 5.7 project for studying the dynamics of solar sails under Earth gravity and solar radiation pressure. The simulation models a sail as a physical actor, tracks its orbital state, detects solar occlusion, and exports telemetry for post-processing and validation.

The project combines:

- C++ simulation actors and physics calculations in Unreal Engine.
- Configurable orbital initialization around Earth.
- Solar-radiation-pressure and Earth-gravity models.
- Multi-ray casting with configurable grid resolution for eclipse and occlusion detection.
- Live telemetry and visual debugging inside the Unreal Editor.
- CSV telemetry export and Python-based plotting tools.
- LaTeX sources for the accompanying thesis.

## Current Features

### Solar-sail dynamics

The `ASolarSail` actor supports configurable sail mass, area, scale, grid resolution, reflectivity, and initial orientation. Multiple sails can be placed in the same simulation with independent parameters.

The simulation can enable or disable Earth gravity and solar radiation pressure independently. At normal time scales, Unreal Engine physics is used to apply the forces to the sail. At higher time scales, the project switches to a manual kinematic integration mode to advance the simulation more efficiently.

### Orbital initialization

The initial orbit can be selected from the Unreal Editor using the existing `EOrbitStartType` options:

- LEO / ISS orbit.
- MEO / GPS orbit.
- Geostationary orbit.
- Lunar-distance orbit.
- Custom altitude.

The initial position, tangential velocity, and orientation are calculated from the selected orbital configuration.

### Solar radiation pressure and occlusion

The `ASimulationManager` calculates solar radiation pressure from the solar luminosity, the sail's distance from the Sun, and the configured force multiplier. The sail samples its surface using an `N × N` ray grid. Each ray is tested toward the Sun to determine whether the corresponding portion of the sail is illuminated or blocked.

For an ideal reflective sail, the pressure contribution follows the usual cosine-squared dependence on the incidence angle:

\[
\vec{F} = 2 P A (\hat{L} \cdot \hat{N})^2 \hat{N}
\]

where:

- `P` is the local solar radiation pressure.
- `A` is the sampled sail area.
- `\hat{L}` is the direction from the sail toward the Sun.
- `\hat{N}` is the sail normal.

The solar pressure decreases with the square of the distance from the Sun:

\[
P = \frac{L}{4 \pi r^2 c}
\]

The implementation also supports reflectivity presets and a custom reflectivity value.

### Telemetry and analysis

When CSV logging is enabled, the simulation writes telemetry to the `Csvs/` directory. The exported data includes values such as time, Earth and Sun distances, velocity, total force, solar pressure, incidence angle, and active photons.

The scripts in `Analysis/` read the generated CSV files and produce plots for orbital expansion, velocity, eclipse validation, and Lambert-law validation.

## Repository Structure

- `Simulation/` — Unreal Engine 5.7 project, C++ source code, configuration, maps, assets, and generated simulation data.
- `Analysis/` — Python scripts, CSV inputs, and generated plots for simulation analysis.
- `Documents/` — Technical references and research papers related to solar sailing, orbital mechanics, physics, and real-time simulation.
- `Thesis/` — LaTeX source files, figures, bibliography, code snippets, and the compiled thesis PDF.
- `DEVLOG.md` — Development history and implementation notes.
- `TODO.md` — Project task list and remaining work.

## Requirements

- Unreal Engine 5.7 or a compatible 5.7 installation.
- A C++ development environment supported by Unreal Engine on the target platform.
- Python with the dependencies used by the analysis scripts, including `pandas`, `matplotlib`, and `numpy`.
- A LaTeX distribution if you want to rebuild the thesis.

## Running the Simulation

1. Clone or download this repository.
2. Open `Simulation/Simulation.uproject` with Unreal Engine 5.7.
3. Open the `MainSimulation` map if it is not selected automatically.
4. Check the `ASimulationManager` and `ASolarSail` properties in the Details panel.
5. Start Play mode and use the available debug and telemetry options to inspect the simulation.
6. If CSV logging is enabled, inspect the generated files under `Simulation/Csvs/` after running the simulation.

The project expects the scene references used by `ASimulationManager`, including the Earth mesh actor, Sun mesh actor, and Sun directional light, to be assigned in the Unreal Editor.

## Running the Analysis Scripts

After generating CSV telemetry from Unreal Engine, run the desired script from the repository root or from the `Analysis/` directory. For example:

```bash
python Analysis/SolarSailPlot.py
```

`SolarSailPlot.py` automatically synchronizes CSV files from `Simulation/Csvs/` and generates plots in `Analysis/Plots/`. Some scripts use the first available CSV file for a specific validation plot.

## Project Status and Limitations

The project is an actively developed research and thesis codebase rather than a packaged production application. The simulation includes Earth gravity, solar radiation pressure, configurable initial orbits, multi-ray occlusion checks, telemetry, and analysis tooling. The remaining work is tracked in `TODO.md`; planned analysis tasks include additional theoretical comparisons and error calculations.

The Unreal project contains binary assets and editor-authored maps, so the simulation must currently be opened and configured through Unreal Engine rather than executed as a standalone command-line program.

## Additional Documentation

- [Development log](DEVLOG.md)
- [Task list](TODO.md)
- [Thesis sources](Thesis/)
- [Analysis scripts](Analysis/)

## License

No open-source license is currently declared for this repository. Until a license is added, the contents remain subject to the author's copyright and should not be reused, modified, or redistributed as open-source software without permission.
