# Devlog

Log di sviluppo del simulatore di vela solare.

---

### Indice
[Update 1: Setup, Bibliografia e Prototipo Iniziale](#update-1-setup-bibliografia-e-prototipo-iniziale)

[Update 2: Architettura Avanzata, Simulation Manager e Multi-Ray Casting](#update-2-architettura-avanzata-simulation-manager-e-multi-ray-casting)

[Update 3: Esportazione Dati, Analisi e Confronto](#update-3-esportazione-dati-analisi-e-confronto)

---

# Update 1: Setup, Bibliografia e Prototipo Iniziale

## Sezioni

### Global
- Aggiunto file [README.md](README.md) con descrizione del progetto.
- Inseriti 2 file per tenere traccia dei progressi fatti ([TODO.md](TODO.md) e [DEVLOG.md](DEVLOG.md)).
- Modificato il `.gitignore` per escludere i file ausiliari del progetto LaTeX [Thesis](Thesis).

### Documents
- Creata cartella delle fonti [Documents](Documents) come raccomandato.
- Aggiunti 7 file (su 22 individuati) che contengono informazioni tecniche utili per la simulazione.

### Presentation
- Aggiunta la cartella [Presentation](Presentation) per quando sarà il momento.

### Simulation
- Creato progetto [Simulation](Simulation) di Unreal Engine 5.7.1.
- Effettuato il setup per VSCode.
- Pulita la simulazione da terreno, nuvole e nebbia per creare il vuoto cosmico.
- Creata la classe C++ principale [`SolarSail.cpp`](Simulation\Source\Simulation\Private\SolarSail.cpp).
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

# Update 2: Architettura Avanzata, Simulation Manager e Multi-Ray Casting

## Sezioni

### Simulation
- Rimossi i commenti, devo imparare come farli migliori.
- Creato una classe C++ [`SimulationManager.cpp`](Simulation/Source/Simulation/Private/SimulationManager.cpp) per i parametri fisici.
- Creato anche il suo header [`SimulationManager.h`](Simulation/Source/Simulation/Public/SimulationManager.h).
- Modificata tutta la fisica semplificata della simulazione, ora la vela viene divisa in una griglia `NxN` che si adatta alla superficie della vela.
- Ogni parte della griglia è autonoma e calcola forza indipendentemente.
- La vela può ora essere spinta da entrambi i lati (prima accadeva per uno solo), invertendo la normale. 
- Modificato interamente l'overlay live per i dati telemetrici.

---

## Risultato Attuale ed Eventuali Errori Irrisolti

> **Stato:** Funzionante
>
> Il simulatore è ora configurato con Multi-Ray Casting, dove la vela è composta da una griglia NxN di celle invece di un singolo punto materiale. Ogni cella della griglia NxN calcola indipendentemente ombre e forza, permettendo la gestione corretta di occlusioni parziali e la conseguente generazione di una rotazione. L'intero sistema è ora centralizzato tramite un Simulation Manager che permette il controllo dei parametri a runtime, mentre un nuovo Overlay mostra in tempo reale lo stato dei singoli raggi (Active o Blocked) e i dati fisici.

---

# Update 3: Esportazione Dati, Analisi e Confronto

## Sezioni

### Simulation
- Implementata l'esportazione automatica dei dati su un file .csv alla fine di ogni simulazione.
- Il file .csv viene aggiornato durante la simulazione ad ogni tick con tutti i dati rilevanti (velocità, forza, angolo di incidenza, ...).

### Analysis
- Creato uno script Python [`SolarSailPlot.py`](Analysis/SolarSailPlot.py) che copia automaticamente il file .csv generato dalla simulazione.
- Lo script genera grafici a partire dai dati contenuti nel file .csv.
- Lo script permette di confrontare più file .csv insieme usando curve con colori diversi.

---

## Risultato Attuale ed Eventuali Errori Irrisolti

> **Stato:** Funzionante
>
> La simulazione ora esporta automaticamente i dati fisici rilevanti in formato .csv, che vengono poi analizzati e confrontati tramite grafici generati dallo script Python dedicato. Questo consente di valutare rapidamente le prestazioni della vela solare e di documentare i risultati in modo chiaro.

---