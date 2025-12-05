#!/bin/bash

echo "[1] Limpando logs antigos..."

rm -f logs/*.csv
rm -f graficos/*.png

echo "[2] Executando simulação..."

gcc -O2 -Wall -o sim sim.c -lm

./sim traces/MPC_times/MPC_long_10/saved_times_long_0.csv > /dev/null 2>&1

if [ ! -f logs/log_runs.csv ]; then
    echo "ERRO: logs/log_runs.csv não foi gerado pela simulação."
    exit 1
fi

echo "[3] Criando diretório de gráficos..."
mkdir -p graficos

echo "[4] Gerando gráficos com gnuplot..."

gnuplot plot_aceitacao.gp
gnuplot plot_comparativo_duplo.gp
gnuplot plot_comparativo.gp
gnuplot plot_load_after.gp
gnuplot plot_load_after_reject.gp
gnuplot plot_load_after_sep.gp
gnuplot plot_load_after_threshold.gp
gnuplot plot_tasks.gp
gnuplot plot_tempo_aceitacao.gp
gnuplot plot_tempo_medio.gp
gnuplot plot_teste.gp

echo "[5] Finalizado!"
echo "Gráficos gerados em: ./graficos/"

