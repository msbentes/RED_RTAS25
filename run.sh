#!/bin/bash

mkdir -p logs
mkdir -p graficos

echo "[1] Limpando logs antigos..."

rm -f log/*.csv
rm -f graficos/*.png

echo "[2] Compilando..."

gcc -O2 -Wall -o sim sim.c -lm


echo "[3] Executando simulação..."


./sim traces/MPC_times/MPC_long_10/saved_times_long_0.csv


if [ ! -f logs/log_runs.csv ]; then
    echo "ERRO: logs/log_runs.csv não foi gerado pela simulação."
    exit 1
fi

echo "[4] Criando diretório de gráficos..."

gnuplot gnuplots/plot_tempo_medio.gp
gnuplot gnuplots/plot_aceitacao.gp
gnuplot gnuplots/plot_comparativo.gp

echo "[5] Finalizado!"
echo "Gráficos gerados em: ./graficos/"



