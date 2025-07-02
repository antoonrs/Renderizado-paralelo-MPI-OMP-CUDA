#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess
import itertools
import csv

# 1) Ruta al ejecutable y directorio de trabajo
script_dir = os.path.dirname(os.path.abspath(__file__))
exe        = os.path.join(script_dir, "mpi_omp_version.exe")

# 2) MPI en Windows
mpi_cmd  = "mpiexec"
mpi_flag = "-n"

# 3) Parámetros a barrer
fotogramas      = [1, 2, 3]
widths          = [32, 256, 512, 1024]
heights         = [32, 256, 512, 1024]
nss             = [1, 3, 10]
nprocs_list     = [1, 4, 8, 16]
threads_list    = [4, 8, 12]
strategies      = ["cols", "rows", "blocks"]
substrategies   = ["cols", "rows", "blocks"]

MAX_FRAMES      = 5

# 4) Fichero de salida CSV
output_file = os.path.join(script_dir, "experimentos.csv")

with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile, delimiter=';')
    # Cabecera
    header = [
        "strategy", "subStrategy",
        "nFotogramas", "width", "height", "ns",
        "nProcs", "threadsPorProceso", "total_time"
    ] + [f"frame_{i}_time" for i in range(MAX_FRAMES)]
    writer.writerow(header)
    csvfile.flush(); os.fsync(csvfile.fileno())

    try:
        # Barrido de todas las combinaciones
        for strat, substrat, nprocs, threads, f, w, h, ns in itertools.product(
            strategies, substrategies,
            nprocs_list,
            threads_list,
            fotogramas,
            widths,
            heights,
            nss
        ):
            cmd = [
                mpi_cmd, mpi_flag, str(nprocs),
                exe,
                str(threads),
                str(f),
                str(w),
                str(h),
                str(ns),
                strat,
                substrat
            ]
            print("Ejecutando:", " ".join(cmd))

            # Ejecutamos el programa y capturamos su salida
            try:
                out = subprocess.check_output(
                    cmd,
                    universal_newlines=True,
                    stderr=subprocess.STDOUT
                )
            except subprocess.CalledProcessError as e:
                print(f"[ERROR] MPI falló en strat={strat}, sub={substrat}, nprocs={nprocs}, threads={threads}, f={f}, w={w}, h={h}, ns={ns}")
                print(e.output)
                continue

            # Extraer la línea CSV generada por el ejecutable
            csv_line = next((l for l in out.splitlines() if l.count(',') >= 5), None)
            if not csv_line:
                print(f"[WARN] No se encontró línea CSV en salida de {cmd}")
                print(out)
                continue

            parts = csv_line.strip().split(',')

            # 1) Total time: lo que imprime el programa (índice 7)
            total_time_str = parts[7].replace('.', ',')

            # 2) Tiempos de cada frame (a partir de índice 8)
            frame_times = parts[8:8 + f]  # f tiempos
            # Rellenar hasta MAX_FRAMES
            frame_times += [""] * (MAX_FRAMES - len(frame_times))
            frame_times_str = [ft.replace('.', ',') if ft else '' for ft in frame_times]

            # Construir la fila definitiva
            row = [
                strat, substrat,
                str(f), str(w), str(h), str(ns),
                str(nprocs), str(threads),
                total_time_str
            ] + frame_times_str

            writer.writerow(row)
            csvfile.flush(); os.fsync(csvfile.fileno())

    except KeyboardInterrupt:
        print("\nInterrumpido por usuario.")
        print(f"Resultados parciales en: {output_file}")
        sys.exit(0)

print("¡Terminado!")
print(f"CSV completo en: {output_file}")
