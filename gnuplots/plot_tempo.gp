set datafile separator ","
set title "Tempo Médio de Execução (ms)"
set xlabel "Run"
set ylabel "Tempo (ms)"
set grid
set terminal pngcairo size 1280,720 enhanced font 'Arial,14'
set output "graficos/tempo.png"

red(x)  = (strcol(2) eq "RED")  ? x : NaN
jams(x) = (strcol(2) eq "JAMS") ? x : NaN

plot "logs/log_runs.csv" skip 1 using 1:(red($4))  with linespoints lt 1 pt 7 lw 2 title "RED", \
     "logs/log_runs.csv" skip 1 using 1:(jams($4)) with linespoints lt 3 pt 7 lw 2 title "JAMS"

