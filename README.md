# Solar Sail Simulator (UE5)

Progetto di Tirocinio / Tesi in Ingegneria Informatica. \
**Studente:** Edoardo Conforti \
**Relatore:** Prof. Franco Milicchio 

---

## Descrizione
Questo progetto implementa un simulatore di vela solare in Unreal Engine 5. L'obiettivo è replicare la fisica della propulsione spaziale basata sulla pressione di radiazione solare (i fotoni che spingono la vela).

Il simulatore calcola la spinta in tempo reale considerando la posizione del Sole, l'orientamento della vela.

## Come funziona
Il cuore della simulazione è la classe C++ `ASolarSail`, che esegue questo ciclo ad ogni fotogramma (Tick):

1.  **Rilevamento Luce (Single-Ray Casting):**
    Il sistema lancia un raggio dal centro della vela verso il Sole. Questo permette di rilevare occlusioni totali e calcolare se la vela è illuminata o in ombra.

2.  **Calcolo della Spinta (Solar Pressure):**
    La forza viene calcolata usando il modello vettoriale di McInnes per vele ideali:
    
    $$ 
    \vec{F} = 2 P A (\hat{L} \cdot \hat{N})^2 \hat{N}
    $$
    
    Dove la pressione di radiazione solare scala con l'inverso del quadrato della distanza:
    
    $$
    P = \frac{P_0}{r^2}
    $$
    
    * $P_0 = 4.56 \times 10^{-6}$ Pa = pressione di radiazione solare a 1 AU
    * $r$ = distanza attuale del Sole (in AU)
    * $A$ = area della vela ($m^2$)
    * $\hat{L}$ = versore della direzione dal Sole alla vela
    * $\hat{N}$ = versore della normale della vela
    * $\alpha = \arccos(\hat{L} \cdot \hat{N})$ = angolo di incidenza della radiazione

3.  **Integrazione Orbitale:**
    Attualmente la simulazione si concentra sulla verifica della spinta fotonica in ambiente isolato (senza gravità).
    Il prossimo step sarà introdurre l'attrazione gravitazionale del Sole per simulare orbite reali.

## Struttura del Progetto
*   `Documents/`: Documentazione tecnica, paper di riferimento e concept.
*   `Simulation/`: Progetto Unreal Engine 5.7 (C++ & Blueprints).
*   `Thesis/`: Sorgenti LaTeX della tesi.
*   `Presentation/`: Materiale per la presentazione finale.

---

## File di supporto

*   [DEVLOG.md](DEVLOG.md): Diario di sviluppo.
*   [TODO.md](TODO.md): Lista delle attività.

---
