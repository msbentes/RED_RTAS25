set datafile separator ","
set term pngcairo size 1280,720 font "Arial,14"
set output "graficos/stats_comparativo.png"

set title "RED vs JAMS — Média, Máximo e Desvio Padrão"
set style data histograms
set style histogram cluster gap 1
set style fill solid 1.0 border -1

set boxwidth 0.8
set grid ytics

set xlabel "Algoritmo"
set ylabel "Tempo (ms)"

plot \
    "stats.csv" using 2:xtic(1) title "Média", \
    "stats.csv" using 3 title "Máximo", \
    "stats.csv" using 5 title "Desvio Padrão"

