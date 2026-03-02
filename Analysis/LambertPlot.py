import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SIMULATION_CSV_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '..', 'Simulation', 'Csvs'))
OUTPUT_PLOT_DIR = os.path.join(SCRIPT_DIR, 'Plots')

# Trova l'ultimo CSV generato
csv_files = glob.glob(os.path.join(SIMULATION_CSV_DIR, "*.csv"))
if not csv_files:
    print("Nessun file CSV trovato!")
    exit()

# Prende il primo file per l'analisi
csv_path = csv_files[0]
data = pd.read_csv(csv_path, skipinitialspace=True)

# Estrazione colonne tramite indice (0=Nome, 1=Tempo, 5=Forza, 7=Angolo, 8=Fotoni)
time = data.iloc[:, 1]
force = data.iloc[:, 5]
angle = data.iloc[:, 7]

# Creazione Grafico a doppio asse Y
fig, ax1 = plt.subplots(figsize=(10, 6))

# Asse Y sinistro: Angolo di incidenza (Rosso)
color1 = 'tab:red'
ax1.set_xlabel('Tempo Simulazione (s)', fontsize=14, fontweight='bold')
ax1.set_ylabel('Angolo di Incidenza (Gradi)', color=color1, fontsize=14, fontweight='bold')
line1 = ax1.plot(time, angle, color=color1, linewidth=2, label='Angolo Incidenza')
ax1.tick_params(axis='y', labelcolor=color1, labelsize=12)
ax1.set_ylim(-10, 190)

# Asse Y destro: Forza Solare (Blu)
ax2 = ax1.twinx()  
color2 = 'tab:blue'
ax2.set_ylabel('Forza Solare Totale (N)', color=color2, fontsize=14, fontweight='bold')
line2 = ax2.plot(time, force, color=color2, linewidth=2, linestyle='--', label='Forza Solare')
ax2.tick_params(axis='y', labelcolor=color2, labelsize=12)

# Titolo e Legenda
plt.title("Validazione Ottica: Legge del Coseno di Lambert", fontsize=18, fontweight='bold')
lines = line1 + line2
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper center', fontsize=12)
ax1.grid(True, linestyle='--', alpha=0.5)

# Salvataggio
os.makedirs(OUTPUT_PLOT_DIR, exist_ok=True)
output_path = os.path.join(OUTPUT_PLOT_DIR, 'Lambert_Validation.png')
plt.tight_layout()
plt.savefig(output_path, dpi=300)
print(f"Grafico Lambert salvato in: {output_path}")

plt.show()