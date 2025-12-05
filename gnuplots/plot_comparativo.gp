set terminal pngcairo size 800,600 enhanced font 'Verdana,12'
set output 'graficos/plot_comparativo.png'

set title "Comparativo RED vs JAMS (100 rodadas)"
set style data histograms
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9
set ylabel "Tempo / Tarefas Aceitas"
set grid ytics

set datafile separator ","   # <- IMPORTANTE!

plot 'logs/dados_comparativos.csv' using 3:xtic(1) title "Tempo Medio", \
     '' using 2:xtic(1) title "Tarefas Aceitas"

