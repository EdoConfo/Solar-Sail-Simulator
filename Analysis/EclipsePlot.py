import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SIMULATION_CSV_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '..', 'Simulation', 'Csvs'))
OUTPUT_PLOT_DIR = os.path.join(SCRIPT_DIR, 'Plots')

# Trova l'ultimo CSV generato
csv_files = glob.glob(os.path.join(SIMULATION_CSV_DIR, "*.csv"))
if not csv_files:
    print("Nessun file CSV trovato!")
    exit()

csv_path = csv_files[0]
data = pd.read_csv(csv_path, skipinitialspace=True)

# Estrazione colonne (1=Tempo, 5=Forza, 8=Fotoni)
time = data.iloc[:, 1]
force = data.iloc[:, 5]
photons = data.iloc[:, 8]

# Creazione Grafico a doppio asse Y
fig, ax1 = plt.subplots(figsize=(12, 6))

# Asse Y sinistro: Forza Solare (Blu)
color1 = 'tab:blue'
ax1.set_xlabel('Tempo Simulazione (s)', fontsize=14, fontweight='bold')
ax1.set_ylabel('Forza Solare Totale (N)', color=color1, fontsize=14, fontweight='bold')
line1 = ax1.plot(time, force, color=color1, linewidth=2, label='Forza Solare')
ax1.tick_params(axis='y', labelcolor=color1, labelsize=12)

# Asse Y destro: Fotoni Attivi (Arancione)
ax2 = ax1.twinx()  
color2 = 'tab:orange'
ax2.set_ylabel('Fotoni Attivi (Ray-Cast)', color=color2, fontsize=14, fontweight='bold')
line2 = ax2.plot(time, photons, color=color2, linewidth=2, linestyle='--', label='Fotoni Attivi')
ax2.tick_params(axis='y', labelcolor=color2, labelsize=12)

# Magia grafica: Evidenzia in grigio le zone di Eclissi (Fotoni = 0)
ax1.fill_between(time, 0, ax1.get_ylim()[1], where=(photons == 0), color='gray', alpha=0.3, label='Zona di Eclissi (Ombra Terra)')

# Titolo e Legenda
plt.title("Validazione Ray-Casting: Transito nelle Zone d'Ombra (Eclissi)", fontsize=18, fontweight='bold')

# Unione delle legende
lines, labels = ax1.get_legend_handles_labels()
lines2, labels2 = ax2.get_legend_handles_labels()
ax1.legend(lines + lines2, labels + labels2, loc='upper right', fontsize=12)

ax1.grid(True, linestyle='--', alpha=0.5)

# Salvataggio
os.makedirs(OUTPUT_PLOT_DIR, exist_ok=True)
output_path = os.path.join(OUTPUT_PLOT_DIR, 'Eclipse_Validation.png')
plt.tight_layout()
plt.savefig(output_path, dpi=300)
print(f"Grafico Eclissi salvato in: {output_path}")

plt.show()