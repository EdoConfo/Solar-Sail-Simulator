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
*   `Simulation/`: Unreal Engine 5.7 project (C++ & Blueprints).
*   `Analysis/`: Python scripts, CSV data and plots used to validate the simulation.
*   `Thesis/`: LaTeX sources of the thesis.

---

## Support files

*   [DEVLOG.md](DEVLOG.md): Development log.
*   [TODO.md](TODO.md): Task list.
*   [REFERENCES.md](REFERENCES.md): Full list of references.

---

## References

Main sources used in this project (see [REFERENCES.md](REFERENCES.md) for the complete list). The source documents are not redistributed in this repository.

- McInnes, Colin R. *Solar Sailing: Technology, Dynamics and Mission Applications.* Springer Praxis Books, 1999.
- Curtis, Howard D. *Orbital Mechanics for Engineering Students.* Elsevier Butterworth-Heinemann, 2005.
- Ericson, Christer. *Real-Time Collision Detection.* CRC Press, 2004.
- Simo, Jules and McInnes, Colin R. "Solar sail trajectory design with a realistic optical model." *Journal of Guidance, Control, and Dynamics,* 2016.
- Spencer, David A., Betts, Bruce, et al. "The LightSail 2 Mission: Flight Results and Lessons Learned." *Acta Astronautica,* 2023.
- Johnson, Les. "Solar Sailing: An Overview." NASA, 2008. https://ntrs.nasa.gov/api/citations/20090019561
- Epic Games. *Unreal Engine 5 Documentation.* https://docs.unrealengine.com/

---

## License

- **Code** (`Simulation/Source/`, `Analysis/*.py`): [MIT License](LICENSE)
- **Thesis, plots and documentation**: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
- **Third-party assets** (`Simulation/Content/Fab/`, `Satellite/`, `Skybox/`) and Unreal Engine remain under their original licenses.

See [LICENSE](LICENSE) for details.
