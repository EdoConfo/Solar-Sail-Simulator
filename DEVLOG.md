# Devlog

Log settimanale di sviluppo del simulatore di vela solare.

---

### Indice
[Settimana 1: Setup, Bibliografia e Prototipo Iniziale](#settimana-1-setup-bibliografia-e-prototipo-iniziale)

---

# Settimana 1: Setup, Bibliografia e Prototipo Iniziale

## Sezioni

### Global
- Aggiunto file [README.md](README.md) con descrizione del progetto.
- Inseriti 2 file per tenere traccia dei progressi fatti ([TODO.md](TODO.md) e [DEVLOG.md](DEVLOG.md)).
- Modificato il `.gitignore` per escludere i file ausiliari del progetto LaTeX [Thesis](Thesis).

### Documents
- Creata cartella delle fonti [Documents](Documents) come raccomandato.
- Aggiunti 7 file (su 22 individuati) che contengono informazioni tecniche utili per la simulazione.

### Presentation
* Aggiunta la cartella [Presentation](Presentation) per quando sarà il momento.

### Simulation
- Creato progetto [Simulation](Simulation) di Unreal Engine 5.7.1.
- Effettuato il setup per VSCode.
- Pulita la simulazione da terreno, nuvole e nebbia per creare il vuoto cosmico.
- Creata la classe C++ principale [`SolarSail`](Simulation\Source\Simulation\Private\SolarSail.cpp).
- Creato anche il suo header [`SolarSail.h`](Simulation\Source\Simulation\Public\SolarSail.h).
- Implementata la logica di ricerca automatica della `DirectionalLight` (Sole) nella scena.
- Sviluppato l'algoritmo di Ray Casting per calcolare l'angolo di incidenza dei raggi solari.
- Applicata la fisica delle forze per simulare la spinta fotonica sulla mesh.

### Thesis
- Aggiunto uno scheletro provvisorio funzionante in LaTeX.

---

## Risultato Attuale ed Eventuali Errori Irrisolti

> **Stato:** Funzionante (Prototipo Base)
>
> Il "Digital Twin" della vela solare è ora operativo all'interno di un livello vuoto. L'Actor C++ implementato rileva la `DirectionalLight` (il Sole) e calcola la spinta fotonica **tramite Ray Casting**. Questo approccio permette di determinare con precisione l'angolo di incidenza dei raggi sulla superficie della vela e applicare la forza risultante lungo la normale, simulando la pressione di radiazione in un ambiente privo di attriti e gravità.

---