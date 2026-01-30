import os
import shutil
import sys
import pandas as pd
import matplotlib.pyplot as plt

csv_files = sys.argv[1:] if len(sys.argv) > 1 else ["SolarSailData.csv"]

if csv_files == ["SolarSailData.csv"]:
    source = os.path.join('..', 'Simulation', 'Saved', 'SolarSailData.csv')
    try:
        shutil.copy2(source, "SolarSailData.csv")
        print(f"File CSV copiato in SolarSailData.csv")
    except Exception as e:
        print(f"Errore nella copia del CSV: {e}")

colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k'] 
plt.figure(figsize=(10,6))

for idx, csv_path in enumerate(csv_files):
    try:
        data = pd.read_csv(csv_path)
        time = data['Time']
        force = (data['ForceX']**2 + data['ForceY']**2 + data['ForceZ']**2)**0.5
        label = os.path.basename(csv_path)
        color = colors[idx % len(colors)]
        plt.plot(time, force, label=label, color=color)
    except Exception as e:
        print(f"Errore con {csv_path}: {e}")

plt.xlabel('Tempo (s)')
plt.ylabel('Forza (N)')
plt.title('Forza fotonica sulla vela nel tempo')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
