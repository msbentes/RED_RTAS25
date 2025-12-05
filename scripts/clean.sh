#!/bin/bash

mkdir -p logs
mkdir -p graficos

echo "[1] Limpando logs e figuras antigos..."

rm -f *.csv
rm -f *.png

rm -f logs/*.csv
rm -f graficos/*.png

rm -f *.o
rm -f red_rosi  red_rosi_logs red_vs_jams1  red_vs_jams  sim

echo "[2] Fim."
