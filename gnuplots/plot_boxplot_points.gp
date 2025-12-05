set datafile separator ","

set title "Boxplot + Pontos - RED vs JAMS"
set xlabel "Algoritmo"
set ylabel "Tempo (ms)"

set style data boxplot
set style boxplot outliers pointtype 7
set style fill solid 0.4
set boxwidth 0.4

set xtics ("RED" 1, "JAMS" 2)

plot \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (1):1 title "RED", \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (2):1 title "JAMS", \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (0.8):1 with points pt 7 lc rgb 'blue' notitle, \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (2.2):1 with points pt 7 lc rgb 'red' notitle

