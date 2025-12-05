set datafile separator ","
set terminal png size 1200,700
set output "graficos/plot_tempo_medio.png"

set title "Tempo Médio de Resposta - RED vs JAMS"
set ylabel "Tempo Médio (ms)"
set xlabel "Algoritmo"

set style data histograms
set style fill solid 1.00 border -1
set boxwidth 0.5

plot "logs/dados_comparativos.csv" using 2:xtic(1) title "Tempo Médio"

