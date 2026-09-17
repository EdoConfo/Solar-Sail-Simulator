# Solar Sail Simulator (UE5)

Internship / Thesis project in Computer Engineering. \
**Student:** Edoardo Conforti \
**Supervisor:** Prof. Franco Milicchio 

---

## Description
This project implements a solar sail simulator in Unreal Engine 5. The goal is to reproduce the physics of space propulsion based on solar radiation pressure (the photons that push the sail).

The simulator computes the thrust in real time, taking into account the position of the Sun and the orientation of the sail.

## How it works
The core of the simulation is the C++ class `ASolarSail`, which runs the following loop every frame (Tick):

1.  **Light Detection (Single-Ray Casting):**
    The system casts a ray from the center of the sail toward the Sun. This makes it possible to detect total occlusions and to compute whether the sail is lit or in shadow.

2.  **Thrust Calculation (Solar Pressure):**
    The force is computed using McInnes' vector model for ideal sails:
    
    $\vec{F} = 2 P A (\hat{L} \cdot \hat{N})^2 \hat{N}$ 
    
    where the solar radiation pressure scales with the inverse square of the distance:
    
    $P = \frac{P_0}{r^2}$ 
    
    * $P_0 = 4.56 \times 10^{-6}$ Pa = solar radiation pressure at 1 AU
    * $r$ = current distance of the Sun (in AU)
    * $A$ = sail area ($m^2$)
    * $\hat{L}$ = unit vector of the direction from the Sun to the sail
    * $\hat{N}$ = unit vector of the sail's normal
    * $\alpha = \arccos(\hat{L} \cdot \hat{N})$ = angle of incidence of the radiation

3.  **Orbital Integration:**
    Currently the simulation focuses on verifying the photonic thrust in an isolated environment (without gravity).
    The next step will be to introduce the Sun's gravitational attraction to simulate real orbits.

## Project Structure
*   `Documents/`: Technical documentation, reference papers and concepts.
*   `Simulation/`: Unreal Engine 5.7 project (C++ & Blueprints).
*   `Thesis/`: LaTeX sources of the thesis.
*   `Presentation/`: Material for the final presentation.

---

## Support files

*   [DEVLOG.md](DEVLOG.md): Development log.
*   [TODO.md](TODO.md): Task list.

---
