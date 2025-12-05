set terminal pngcairo size 1200,600 enhanced font 'Verdana,12'
set output 'graficos/comparativo_red_jams_duplo.png'

set multiplot layout 1,2 title "Comparativo RED vs JAMS (100 rodadas)" font ",14"
set datafile separator ","  # <- IMPORTANTE!

# Grafico 1
set title "Tempo Medio de Resposta (ms)"
set ylabel "Tempo Medio (ms)"
set style data histograms
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.6
set grid ytics
plot 'logs/dados_comparativos.csv' using 3:xtic(1) title "Tempo Medio"

# Grafico 2
set title "Tarefas Aceitas"
set ylabel "Numero de Tarefas"
set style data histograms
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.6
set grid ytics
plot 'logs/dados_comparativos.csv' using 2:xtic(1) title "Tarefas Aceitas"

unset multiplot

