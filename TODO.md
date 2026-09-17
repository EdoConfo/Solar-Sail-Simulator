# ToDo List

TODO list organized by project directory.

---


# 1. Simulation

- [x] Create the base C++ class `SolarSail`.
- [x] Create a mirrored material to make it look like a reflective sail.
- [x] Disable gravity and friction.
- [x] Make the code find the DirectionalLight (Sun) on its own.
- [x] Implement a Ray Casting algorithm to compute the irradiance on the surface.
- [x] Check whether there is something in between casting a shadow (planets, asteroids).
- [x] Compute how tilted the sail is with respect to the light.
- [x] Apply the thrust in the right direction (along the normal).
- [x] Draw colored lines to see the ray and the force.
- [x] Improve the physics simulation (Multi-Ray Grid and Simulation Manager).
- [x] Save position and velocity to a CSV file for the plots.
- [x] Attach the sail to a satellite (I have doubts, we'll see later)
- [x] The thrust only happens on one face of the sail... Fix it
- [x] Add a reflection factor to the `SolarSail` class to handle non-ideal cases (currently fixed at 2) and different for each sail.
- [x] Animations of Earth and Sun (rotation, revolution) to make everything more realistic and calibrated to the TimeScale.
- [x] Fix the problem with the timescale, maybe it's the max clamp that is too low (currently 1000x).
- [x] Fix the problem with the angle of incidence that seems to always be 0° (maybe a problem with vector normalization or with the sail's orientation).
- [x] Test the simulation with multiple sails (with different mass) to see whether they behave as expected (same force, different acceleration).
- [x] Orbit not fixed but editable in the editor.
- [x] Completely disable gravity and/or solar force to test only one of the two (currently it is only possible to scale the force, but not to disable it entirely). (If I disable gravity, does the sail keep rotating anyway??)
- [x] Add the ability to have different orbits for different sails.
- [x] Fix the categories of the UPROPERTY to better organize the parameters in the editor (e.g. "Orbit", "Sail", "Simulation").
- [x] Implementation of gravity and stable orbit
	- [x] Define the position of the Earth (e.g. origin, FVector::ZeroVector)
	- [x] Add the gravitational constant, the Earth's mass and the sail's mass to the simulation parameters
	- [x] Create a function to compute the gravitational force on the sail, using the formula:
		$F = G \cdot \frac{M_{earth} \cdot m_{sail}}{r^2}$
	- [x] Integrate the gravitational force into the sail's Tick()
	- [x] Set the sail's initial position on the geostationary orbit (correct radius)
	- [x] Compute and set the initial tangential orbital velocity (3.07 km/s) (TO BE TESTED AGAIN, THERE MAY HAVE BEEN A PROBLEM WITH THE TIME SCALE)
	- [x] Force the sail's orientation: normal always perpendicular to the Earth-sail vector (angle 0°)
	- [x] Update the sail's rotation at every tick to maintain the angle
	- [x] Verify that solar pressure and gravity act correctly together
- [x] Tidy up the detail panel of the Simulation Manager and of the sail, hiding the parameters the user doesn't need and grouping the useful ones into logical categories (e.g. "Orbit", "Sail", "Simulation").
- [x] Fix the solar pressure rays that sometimes seem not to hit the sail (maybe a collision problem or a problem with the rays' orientation).
- [x] Redraw the live telemetry.
- [x] Fix the problem with solar pressure.
- [x] UPROPERTY not editable at runtime (e.g. the sail's mass) to avoid simulation problems.
- [x] Fix the crash when modifying parameters at runtime (maybe a simulation reset or better parameter handling is needed).

---

# 2. Analysis

- [x] Generation of plots from the Unreal simulation data (python script)
- [x] Ability to compare multiple CSVs on the same plot (different colors)
- [x] Generation of plots from the ideal formulas
- [x] Comparison between real, theoretical and simulated plots
- [x] Computation of the actual error

---

# 3. Thesis

- [x] Working LaTeX project setup.
- [x] Writing (duh).
- [x] Review.
- [x] Review.
- [x] Review.

---

# 4. Presentation

- [x] Record a video of the simulation.
- [x] Make the presentation (no way).
- [x] Write the speech.

---
