import os
import shutil
import sys
import pandas as pd
import matplotlib.pyplot as plt
import glob

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SIMULATION_CSV_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '..', 'Simulation', 'Csvs'))
LOCAL_CSV_DIR = os.path.join(SCRIPT_DIR, 'Csvs')
OUTPUT_PLOT_DIR = os.path.join(SCRIPT_DIR, 'Plots')

def get_csv_files():
    """Gestisce la logica di copia e recupero file."""
    files_to_process = []
    
    # CASO A: Nessun argomento -> MODALITÀ AUTOMATICA (Sync & Plot All)
    if len(sys.argv) == 1:
        print(f"--- Modalita' Automatica ---")
        print(f"Sorgente dati: {SIMULATION_CSV_DIR}")
        
        # 1. Pulisce la cartella locale per evitare dati vecchi
        if os.path.exists(LOCAL_CSV_DIR):
            shutil.rmtree(LOCAL_CSV_DIR)
        os.makedirs(LOCAL_CSV_DIR)
        
        # 2. Copia i file dalla simulazione
        if os.path.exists(SIMULATION_CSV_DIR):
            sim_files = glob.glob(os.path.join(SIMULATION_CSV_DIR, "*.csv"))
            
            if not sim_files:
                print("NESSUN CSV TROVATO nella cartella di simulazione!")
                return []
                
            for file_path in sim_files:
                file_name = os.path.basename(file_path)
                dest_path = os.path.join(LOCAL_CSV_DIR, file_name)
                shutil.copy2(file_path, dest_path)
                files_to_process.append(dest_path)
                print(f"Copiato: {file_name}")
        else:
            print(f"Errore: La cartella {SIMULATION_CSV_DIR} non esiste. Hai avviato la simulazione?")
            return []
            
    # CASO B: Argomenti passati -> MODALITÀ MANUALE (Plotta solo i file richiesti)
    else:
        print(f"--- Modalita' Manuale ---")
        files_to_process = sys.argv[1:]

    return files_to_process

def plot_data(csv_files):
    if not csv_files:
        print("Nessun file da graficare.")
        return

    # Creiamo una figura con 2 grafici impilati (Distanza e Velocità)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)
    
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k'] # Colori ciclici per le diverse vele

    for idx, csv_path in enumerate(csv_files):
        try:
            # Legge il CSV (gestisce eventuali spazi dopo le virgole)
            data = pd.read_csv(csv_path, skipinitialspace=True)
            
            # Pulisce il nome per la legenda (toglie "SolarSailData_" e ".csv")
            label = os.path.basename(csv_path).replace("SolarSailData_", "").replace(".csv", "")
            color = colors[idx % len(colors)]

            # --- ESTRAZIONE DATI ---
            # Assicurati che questi nomi coincidano con l'Header nel C++!
            time = data['Timestamp(s)']
            dist_earth = data['DistEarth(Km)']
            velocity = data['Velocity(Km/s)']
            
            # Grafico 1: Distanza dalla Terra
            ax1.plot(time, dist_earth, label=label, color=color, linewidth=2)
            
            # Grafico 2: Velocità
            ax2.plot(time, velocity, label=label, color=color, linestyle='--', linewidth=1.5)

        except Exception as e:
            print(f"Errore leggendo {csv_path}: {e}")
            continue

    # --- FORMATTAZIONE GRAFICI ---
    
    # Grafico Superiore (Distanza)
    ax1.set_ylabel('Distanza Terra (Km)', fontsize=12)
    ax1.set_title('Analisi Orbita Vela Solare', fontsize=14)
    ax1.grid(True, which='both', linestyle='--', alpha=0.7)
    ax1.legend(loc='upper left')

    # Grafico Inferiore (Velocità)
    ax2.set_xlabel('Tempo Simulazione (s)', fontsize=12)
    ax2.set_ylabel('Velocità (Km/s)', fontsize=12)
    ax2.grid(True, which='both', linestyle='--', alpha=0.7)

    # Salvataggio su file
    os.makedirs(OUTPUT_PLOT_DIR, exist_ok=True)
    output_path = os.path.join(OUTPUT_PLOT_DIR, 'SolarSailData_Plot.png')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    print(f"Grafico salvato in: {output_path}")
    
    # Mostra a schermo
    plt.show()

if __name__ == "__main__":
    files = get_csv_files()
    plot_data(files)