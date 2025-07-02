#!/usr/bin/env python3
"""
plot_results.py

Lee un CSV de resultados de renderizado y genera comparativas de avg_frame_time
and total_time frente a cada variable: strategy, nFotogramas, width, height, ns y nProcs.
Guarda todos los resultados en la subcarpeta 'plots':
  - gráficos de barras con la media de cada métrica por variable
  - boxplots para visualizar distribución
  - diagramas de quesitos de recuentos y medias
  - estadísticas descriptivas (media, mediana, std) en CSV
"""

import sys
import os
import pandas as pd
import matplotlib.pyplot as plt


def parse_args(argv):
    csv_file = 'experimentos.csv'
    if len(argv) >= 2 and not argv[1].isdigit():
        csv_file = argv[1]
    return csv_file


def main():
    # 1) Parámetro: CSV
    csv_file = parse_args(sys.argv)
    if not os.path.isfile(csv_file):
        print(f"Error: no existe '{csv_file}'")
        sys.exit(1)

    # 2) Preparar directorio de salida
    output_dir = 'plots'
    os.makedirs(output_dir, exist_ok=True)

    # 3) Leer CSV (punto decimal coma)
    df = pd.read_csv(csv_file, sep=';', decimal=',')

    # 4) Asegurar columnas numéricas
    frame_cols = [c for c in df.columns if c.startswith('frame_')]
    df[frame_cols] = df[frame_cols].apply(pd.to_numeric, errors='coerce')
    df['avg_frame_time'] = df[frame_cols].mean(axis=1)
    df['total_time']      = pd.to_numeric(df['total_time'], errors='coerce')

    # 5) Variables y métricas
    variables = {
        'strategy': ['cols', 'rows', 'blocks'],
        'nFotogramas': sorted(df['nFotogramas'].unique()),
        'width':        sorted(df['width'].unique()),
        'height':       sorted(df['height'].unique()),
        'ns':           sorted(df['ns'].unique()),
        'nProcs':       sorted(df['nProcs'].unique()),
    }
    metrics = ['avg_frame_time', 'total_time']

        # 6) Diagramas de líneas originales por variable (tiempo vs índice de ejecución)
    for var in variables.keys():
        for metric in metrics:
            plt.figure()
            for val in variables[var]:
                subset = df[df[var] == val]
                if subset.empty:
                    continue
                plt.plot(
                    subset.index,
                    subset[metric],
                    label=str(val),
                    alpha=0.7,
                    linewidth=1
                )
            plt.title(f'{metric.replace("_"," ").title()} por {var} (línea)')
            plt.xlabel('Índice de ejecución')
            plt.ylabel(metric.replace('_',' ').title())
            plt.legend(title=var)
            plt.tight_layout()
            fn_line = os.path.join(output_dir, f'line_{metric}_por_{var}.png')
            plt.savefig(fn_line)
            plt.close()
            print(f"Guardado: {fn_line}")

    # 7) Gráficos de barras (media) y boxplots por variable
    for var, vals in variables.items():
        for metric in metrics:
            grouped = df.groupby(var)[metric]
            means = grouped.mean().reindex(vals, fill_value=0)
            # Gráfico de barras
            plt.figure()
            plt.bar([str(v) for v in vals], means)
            plt.title(f'Media de {metric.replace("_"," ")} por {var}')
            plt.xlabel(var)
            plt.ylabel(metric.replace('_',' ').title())
            fn_bar = os.path.join(output_dir, f'barras_{metric}_por_{var}.png')
            plt.tight_layout()
            plt.savefig(fn_bar)
            plt.close()
            print(f"Guardado: {fn_bar}")

            # Boxplot de distribución
            data = [df[df[var]==v][metric].dropna() for v in vals]
            plt.figure()
            plt.boxplot(data, labels=[str(v) for v in vals], showfliers=False)
            plt.title(f'Distribución de {metric.replace("_"," ")} por {var}')
            plt.xlabel(var)
            plt.ylabel(metric.replace('_',' ').title())
            fn_box = os.path.join(output_dir, f'boxplot_{metric}_por_{var}.png')
            plt.tight_layout()
            plt.savefig(fn_box)
            plt.close()
            print(f"Guardado: {fn_box}")

    # 7) Diagramas de quesitos: recuentos y medias
    for var in variables.keys():
        counts = df[var].value_counts().reindex(variables[var], fill_value=0)
        plt.figure()
        plt.pie(counts, labels=counts.index, autopct='%1.1f%%')
        plt.title(f'Distribución de ejecuciones por {var}')
        fn_pie1 = os.path.join(output_dir, f'pie_counts_{var}.png')
        plt.savefig(fn_pie1)
        plt.close()
        print(f"Guardado: {fn_pie1}")

        for metric in metrics:
            means = df.groupby(var)[metric].mean().reindex(variables[var], fill_value=0)
            plt.figure()
            plt.pie(means, labels=means.index, autopct='%1.1f%%')
            plt.title(f'Media de {metric.replace("_"," ")} por {var}')
            fn_pie2 = os.path.join(output_dir, f'pie_means_{metric}_por_{var}.png')
            plt.savefig(fn_pie2)
            plt.close()
            print(f"Guardado: {fn_pie2}")

    # 8) Estadísticas descriptivas por grupo: mean, median, std
    for var in variables.keys():
        for metric in metrics:
            stats = df.groupby(var)[metric].agg(['mean','median','std']).reindex(variables[var])
            csv_stats = os.path.join(output_dir, f'estadisticas_{metric}_por_{var}.csv')
            stats.to_csv(csv_stats, sep=';')
            print(f"Guardado: {csv_stats}")


if __name__ == '__main__':
    main()
