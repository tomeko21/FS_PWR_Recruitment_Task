import os
import subprocess
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# Konfiguracja ścieżek
CPP_SRC = "zadanie2_2.cpp"
CPP_EXEC = "./zadanie2_2"
SENSOR1_PATH = "./tests/data/sensor1.csv"
SENSOR2_PATH = "./tests/data/sensor2.csv"

# kompilacja jesli nie ma pliku skompilowanego
if not os.path.exists(CPP_EXEC):
    compile_cmd = ["g++", "-O2", CPP_SRC, "-o", CPP_EXEC]
    subprocess.run(compile_cmd, check=True)


df_s1 = pd.read_csv(SENSOR1_PATH)
df_s2 = pd.read_csv(SENSOR2_PATH)

# Odwracam oś Y sensora 2
s1_x, s1_y = df_s1["x"].values, df_s1["y"].values
s2_x, s2_y = df_s2["x"].values, -df_s2["y"].values

# 3. Wygenerowanie punktów co 20 ms
dt = 0.02  
t_max = min(df_s1["timestamp"].max(), 33.0)
timestamps = np.arange(0.0, t_max + dt / 2, dt)

# Przygotowanie ciągu tekstowego z czasami dla C++  (stdin)
input_data = "\n".join(f"{t:.3f}" for t in timestamps) + "\n"

# Uruchomienie programu c++
proc = subprocess.Popen(
    [CPP_EXEC, SENSOR1_PATH, SENSOR2_PATH],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
)

stdout_data, _ = proc.communicate(input=input_data)

# Odczytanie wygenerowanych punktów z wyjścia programu C++
fused_x, fused_y = [], []
for line in stdout_data.strip().split("\n"):
    if line.strip():
        parts = line.strip().split()
        if len(parts) == 2:
            fused_x.append(float(parts[0]))
            fused_y.append(float(parts[1]))

# Wykres
plt.figure(figsize=(14, 9))

# Sensor 1
plt.plot(
    s1_x,
    s1_y,
    label="Sensor 1 (odometria)",
    color="red",
    linestyle="--",
    linewidth=1.5,
    alpha=0.6,
)

# Sensor 2 
plt.scatter(
    s2_x,
    s2_y,
    label="Sensor 2 (GPS)",
    color="blue",
    s=35,
    zorder=3,
    alpha=0.8,
)

# Wyniki programu
plt.plot(
    fused_x,
    fused_y,
    label="Wynik C++ (Krok 20 ms)",
    color="green",
    linewidth=2.5,
    zorder=4,
)

# Stylizacja wykresu
plt.xlabel("Pozycja X [m]", fontsize=12)
plt.ylabel("Pozycja Y [m]", fontsize=12)
plt.legend(fontsize=11, loc="best")
plt.grid(True, linestyle=":", alpha=0.6)
plt.gca().set_aspect("equal", adjustable="box")

plt.tight_layout()

# Zapisanie wykresu do pliku i wyświetlenie
plt.savefig("wynik_zad2.png", dpi=300)
plt.show()