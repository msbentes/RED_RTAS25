set terminal pngcairo size 1200,800 enhanced font "Arial,14"
set output "graficos/boxplot_red_jams.png"

set datafile separator ","
set title "Boxplot - Tempo de Execução (ms)"
set xlabel "Algoritmo"
set ylabel "Tempo (ms)"

set style data boxplot
set style boxplot outliers pointtype 7
set style fill solid 0.5
set boxwidth 0.6

set xtics ("RED" 1, "JAMS" 2)

plot \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (1):1 title "RED", \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (2):1 title "JAMS"

