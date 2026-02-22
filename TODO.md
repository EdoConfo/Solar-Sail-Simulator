# ToDo List

Lista dei TODO organizzata per directory di progetto.

---


# 1. Simulation

- [x] Creare la classe C++ base `SolarSail`.
- [x] Creare un materiale specchiato per farla sembrare una vela riflettente.
- [x] Disabilitare la gravità e attrito.
- [x] Fare in modo che il codice trovi da solo la DirectionalLight (Sole).
- [x] Implementare algoritmo di Ray Casting per calcolare l'irradianza sulla superficie.
- [x] Controllare se c'è qualcosa in mezzo che fa ombra (pianeti, asteroidi).
- [x] Calcolare quanto è inclinata la vela rispetto alla luce.
- [x] Applicare la spinta nella direzione giusta (lungo la normale).
- [x] Disegnare linee colorate per vedere raggio e forza.
- [x] Migliorare la simulazione fisica (Griglia Multi-Ray e Simulation Manager).
- [x] Salvare posizione e velocità su un file CSV per i grafici.
- [x] Attaccare la vela ad un satellite (Ho dei dubbi, vedremo dopo)
- [x] La spinta avviene solo da una faccia della vela... Risolvi
- [x] Aggiungere fattore di riflessione alla classe `SolarSail` per gestire casi non ideali (attualmente è 2 fisso) e diversi per ogni vela.
- [x] Animazioni di Terra e Sole (rotazione, rivoluzione) per rendere tutto più realistico e calibrato sul TimeScale.
- [x] Risolvi problema con timescale, forse è il clamp max che è troppo basso (attualmente 1000x).
- [x] Risolvi il problema dell'angolo di incidenza che sembra essere sempre 0° (forse un problema di normalizzazione dei vettori o di orientamento della vela).
- [x] Testa simulazione con più vele (con massa diversa) per vedere se si comportano come previsto (stessa forza, accelerazione diversa).
- [x] Orbita non fissa ma modificabile nell'editor.
- [x] Disattivare completamente gravità e/o forza solare per testare solo una delle due (attualmente è possibile solo scalare la forza, ma non disattivarla del tutto). (Se disattivo la gravità la vela continua a girarsi comunque??)
- [x] Aggiungi possibilità di avere diverse orbite per diverse vele.
- [x] Sistema le category degli UPROPERTY per organizzare meglio i parametri nell'editor (es. "Orbita", "Vela", "Simulazione").
- [x] Implementazione gravità e orbita stabile
	- [x] Definire la posizione della Terra (es. origine, FVector::ZeroVector)
	- [x] Aggiungere costante gravitazionale, massa della Terra e massa della vela nei parametri di simulazione
	- [x] Creare funzione per calcolare la forza gravitazionale sulla vela, usando la formula:
		$F = G \cdot \frac{M_{terra} \cdot m_{vela}}{r^2}$
	- [x] Integrare la forza gravitazionale nel Tick() della vela
	- [x] Impostare la posizione iniziale della vela sull'orbita geostazionaria (raggio corretto)
	- [x] Calcolare e impostare la velocità orbitale iniziale tangente (3,07 km/s) (DA TESTARE NUOVAMENTE, FORSE C'ERA UN PROBLEMA CON IL TIME SCALE)
	- [x] Forzare l'orientamento della vela: normale sempre perpendicolare al vettore Terra-vela (angolo 0°)
	- [x] Aggiornare la rotazione della vela a ogni tick per mantenere l'angolo
	- [x] Verificare che la pressione solare e la gravità agiscano correttamente insieme
- [x] Rendi ordinato il detail del Simulation Manager e della vela, nascondendo i parametri che non servono all'utente e raggruppando quelli che servono in categorie logiche (es. "Orbita", "Vela", "Simulazione").
- [x] Sistema i raggi della pressione solare che a volte sembrano non colpire la vela (forse un problema di collisioni o di orientamento dei raggi).
- [x] Ridisegna live telemetry.
- [x] Risolvi problema con pressione solare.
- [x] UPROPERTY non modificabili in runtime (es. massa della vela) per evitare problemi di simulazione.
- [x] Risolvi crash alla modifica dei parametri in runtime (forse è necessario un reset della simulazione o una gestione migliore dei parametri).

---

# 2. Analysis

- [x] Generazione grafici a partire dai dati della simulazione di Unreal (script python)
- [x] Possibilità di confrontare più CSV sullo stesso grafico (colori diversi)
- [ ] Generazione grafici a partire dalle formule ideali
- [ ] Confronto tra grafici reali, teorici e simulati
- [ ] Calcolo dell'errore effettivo

---

# 3. Thesis

- [x] Setup del progetto LaTeX funzionante.
- [ ] Scrittura (duh).
- [ ] Revisione.
- [ ] Revisione.
- [ ] Revisione.

---

# 4. Presentation

- [ ] Registra video della simulazione.
- [ ] Fai presentazione (no way).
- [ ] Scrivi il discorso.

---
