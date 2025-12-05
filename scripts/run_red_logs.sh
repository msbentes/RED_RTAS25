#!/bin/bash

mkdir -p logs
mkdir -p graficos

echo "[1] Limpando logs antigos..."

rm -f log/*.csv
rm -f graficos/*.png

echo "[2] Executando simulação..."

gcc -O2 -Wall -o red_rosi_logs red_rosi_logs.c -lm

./red_rosi_logs > /dev/null 2>&1

if [ ! -f logs/log_tarefas.csv ]; then
    echo "ERRO: logs/log_runs.csv não foi gerado pela simulação."
    exit 1
fi

echo "[3] Criando diretório de gráficos..."

gnuplot gnuplots/plot_load.gp


echo "[5] Finalizado!"
echo "Gráficos gerados em: ./graficos/"

