#!/usr/bin/env python3
import os
import sys
import subprocess
import itertools
import csv
import time

# 1) Ruta al ejecutable y directorio de trabajo
script_dir = os.path.dirname(os.path.abspath(__file__))
exe        = os.path.join(script_dir, "mpi_version.exe")

# 2) MPI en Windows
mpi_cmd  = "mpiexec"
mpi_flag = "-n"

# 3) Parámetros
fotogramas  = [1, 2, 5]
widths      = [32, 64, 256, 512, 1024]
heights     = [32, 64, 256, 512, 1024]
nss         = [1, 2, 5, 10]
nprocs_list = [1, 2, 4, 8, 16]
strategies  = ["cols", "rows", "blocks"]

MAX_FRAMES  = 5

# 4) Fichero de salida en el propio directorio
output_file = os.path.join(script_dir, "experimentos.csv")

with open(output_file, "w", newline="") as csvfile:
    # Usamos QUOTE_ALL para que todos los campos vayan entre comillas y Excel no reinterprete decimales
    writer = csv.writer(csvfile, delimiter=';')  # Usamos ';' y coma decimal manualmente
    # Cabecera
    header = [
        "strategy", "nFotogramas", "width", "height", "ns", "nProcs", "total_time"
    ] + [f"frame_{i}_time" for i in range(MAX_FRAMES)]
    writer.writerow(header)
    csvfile.flush(); os.fsync(csvfile.fileno())

    try:
        for strat in strategies:
            for nprocs in nprocs_list:
                for f, w, h, ns in itertools.product(fotogramas, widths, heights, nss):
                    cmd = [
                        mpi_cmd, mpi_flag, str(nprocs),
                        exe, str(f), str(w), str(h), str(ns), strat
                    ]
                    print("Ejecutando:", " ".join(cmd))

                    start_time = time.time()
                    try:
                        out = subprocess.check_output(
                            cmd,
                            universal_newlines=True,
                            stderr=subprocess.STDOUT
                        )
                    except subprocess.CalledProcessError as e:
                        print(f"[ERROR] MPI falló en strat={strat}, nprocs={nprocs}, f={f}, w={w}, h={h}, ns={ns}")
                        print(e.output)
                        continue
                    end_time = time.time()
                    real_duration = end_time - start_time

                    # Extraer la línea CSV generada por el ejecutable
                    csv_line = next((l for l in out.splitlines() if l.count(',') >= 5), None)
                    if not csv_line:
                        print(f"[WARN] No se encontró línea CSV en salida de {cmd}")
                        print(out)
                        continue

                    parts       = csv_line.strip().split(',')
                    # Envolvemos total_time entre comillas manualmente para asegurar formato (opcional si usamos QUOTE_ALL)
                    total_time  = parts[5]
                    frame_times = parts[6:6 + f] + [""] * (MAX_FRAMES - f)

                    # Debug: mostrar tiempos interpretados vs medidos
                    print(f"[DEBUG] total_time parsed: {total_time}s | measured wall-clock: {real_duration:.3f}s")

                                        # Convertir punto decimal a coma para Excel español
                    total_time = total_time.replace('.', ',')
                    frame_times = [ft.replace('.', ',') if ft else '' for ft in frame_times]

                    # Construir la fila (sin comillas extras)
                    row = [
                        strat, str(f), str(w), str(h), str(ns),
                        str(nprocs), total_time
                    ] + frame_times

                    writer.writerow(row)
                    csvfile.flush(); os.fsync(csvfile.fileno())

    except KeyboardInterrupt:
        print(f"\nInterrumpido por usuario.")
        print(f"Resultados parciales en: {output_file}")
        sys.exit(0)

print("¡Terminado!")
print(f"CSV completo en: {output_file}")
