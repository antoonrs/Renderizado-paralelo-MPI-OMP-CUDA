#!/usr/bin/env python3
import os
import sys
import subprocess
import itertools
import csv

# 1) Ruta al ejecutable y directorio de trabajo
script_dir = os.path.dirname(os.path.abspath(__file__))
exe = os.path.join(script_dir, "cuda_version.exe")

# 2) Parámetros de barrido
widths        = [32, 64, 256, 512, 1024]
heights       = [32, 64, 256, 512, 1024]
ns_list       = [1, 2, 5, 10]
threadsX_list = [1, 4, 8, 16, 32]
threadsY_list = [1, 4, 8, 16, 32]

# 3) Archivo de salida
output_file = os.path.join(script_dir, "experimentos.csv")

# Límite de threads por bloque en CUDA (generalmente 1024)
MAX_THREADS_PER_BLOCK = 1024

with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile, delimiter=';')
    writer.writerow(["threadsX", "threadsY", "width", "height", "ns", "timer_seconds"])
    csvfile.flush()
    os.fsync(csvfile.fileno())

    try:
        for w, h, ns, tx, ty in itertools.product(
                widths,
                heights,
                ns_list,
                threadsX_list,
                threadsY_list
            ):
            # Aunque la configuración ahora siempre sea válida, mantenemos la comprobación
            if tx * ty > MAX_THREADS_PER_BLOCK:
                continue

            cmd = [exe, str(w), str(h), str(ns), str(tx), str(ty)]
            print("Ejecutando:", " ".join(cmd))

            try:
                out = subprocess.check_output(
                    cmd,
                    universal_newlines=True,
                    stderr=subprocess.STDOUT
                )
            except subprocess.CalledProcessError as e:
                print(f"[ERROR] fallo en w={w}, h={h}, ns={ns}, tx={tx}, ty={ty}")
                print(e.output)
                continue

            parts = out.strip().split(',')
            if len(parts) != 6:
                print(f"[WARNING] salida inesperada ({len(parts)} campos): '{out.strip()}'")
                continue

            tx_out, ty_out, w_out, h_out, ns_out, timer_sec = parts

            writer.writerow([tx_out, ty_out, w_out, h_out, ns_out, timer_sec])
            csvfile.flush()
            os.fsync(csvfile.fileno())

    except KeyboardInterrupt:
        print("\nInterrumpido por usuario.")
        print(f"Resultados parciales en: {output_file}")
        sys.exit(0)

print("¡Terminado!")
print(f"CSV completo en: {output_file}")
