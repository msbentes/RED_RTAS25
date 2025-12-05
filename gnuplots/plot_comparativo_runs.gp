set datafile separator ","
set term pngcairo size 1280,720 font "Arial,14"
set output "graficos/comparativo_runs.png"

set title "Comparação RED vs JAMS por Run"
set xlabel "Run"
set ylabel "Tempo (ms)"
set grid

red(x)  = (strcol(2) eq "RED")  ? x : NaN
jams(x) = (strcol(2) eq "JAMS") ? x : NaN

plot "logs/log_runs.csv" skip 1 using 1:(red($4))  with linespoints lw 2 pt 7 title "RED", \
     "logs/log_runs.csv" skip 1 using 1:(jams($4)) with linespoints lw 2 pt 7 title "JAMS"

