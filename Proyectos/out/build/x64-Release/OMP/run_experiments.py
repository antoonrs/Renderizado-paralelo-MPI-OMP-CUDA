#!/usr/bin/env python3
import os
import sys
import subprocess
import itertools
import csv
import time

# 1) Ruta al ejecutable y directorio de trabajo
script_dir   = os.path.dirname(os.path.abspath(__file__))
exe          = os.path.join(script_dir, "omp_version.exe")

# 2) Parámetros a variar
fotogramas    = [1, 2, 5]
widths        = [32, 64, 256, 512, 1024]
heights       = [32, 64, 256, 512, 1024]
nss           = [1, 2, 5, 10]
nthreads_list = [1, 2, 4, 8, 16]
strategies    = ["cols", "rows", "blocks"]

MAX_FRAMES   = 5

# 3) Fichero de salida en el propio directorio
output_file = os.path.join(script_dir, "experimentos_omp.csv")

with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile, delimiter=';')
    # Cabecera
    header = [
        "strategy", "nFotogramas", "width", "height", "ns", "nThreads", "total_time"
    ] + [f"frame_{i}_time" for i in range(MAX_FRAMES)]
    writer.writerow(header)
    csvfile.flush()
    os.fsync(csvfile.fileno())

    try:
        for strat in strategies:
            for nthreads in nthreads_list:
                for f, w, h, ns in itertools.product(fotogramas, widths, heights, nss):
                    # Construir comando: pasamos totalThreads como argv[1]
                    cmd = [
                        exe,
                        str(nthreads),
                        str(f),
                        str(w),
                        str(h),
                        str(ns),
                        strat
                    ]
                    print("Ejecutando:", " ".join(cmd))

                    # Fijar OMP_NUM_THREADS por si el programa usa omp_get_max_threads()
                    env = os.environ.copy()
                    env["OMP_NUM_THREADS"] = str(nthreads)

                    start_time = time.time()
                    try:
                        out = subprocess.check_output(
                            cmd,
                            env=env,
                            universal_newlines=True,
                            stderr=subprocess.STDOUT
                        )
                    except subprocess.CalledProcessError as e:
                        print(f"[ERROR] OMP falló en strat={strat}, nthreads={nthreads}, "
                              f"f={f}, w={w}, h={h}, ns={ns}")
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
                    total_time  = parts[5]
                    frame_times = parts[6:6 + f] + [""] * (MAX_FRAMES - f)

                    # Debug: tiempos interpretados vs medidos
                    print(f"[DEBUG] total_time parsed: {total_time}s | "
                          f"measured wall-clock: {real_duration:.3f}s")

                    # Convertir punto decimal a coma para Excel español
                    total_time  = total_time.replace('.', ',')
                    frame_times = [ft.replace('.', ',') if ft else '' for ft in frame_times]

                    # Construir la fila
                    row = [
                        strat,
                        str(f),
                        str(w),
                        str(h),
                        str(ns),
                        str(nthreads),
                        total_time
                    ] + frame_times

                    writer.writerow(row)
                    csvfile.flush()
                    os.fsync(csvfile.fileno())

    except KeyboardInterrupt:
        print("\nInterrumpido por usuario.")
        print(f"Resultados parciales en: {output_file}")
        sys.exit(0)

print("¡Terminado!")
print(f"CSV completo en: {output_file}")
