set datafile separator ","
set term pngcairo size 1280,720 font "Arial,14"
set output "graficos/cdf_accepted.png"

set title "CDF — Número de Tarefas Aceitas"
set xlabel "Tarefas Aceitas"
set ylabel "F(x)"
set grid

red(x)  = (strcol(2) eq "RED")  ? x : NaN
jams(x) = (strcol(2) eq "JAMS") ? x : NaN

plot \
    "logs/log_runs.csv" skip 1 using (red($3)):(1) smooth cumulative \
        with lines lw 3 lc rgb "blue" title "RED", \
    "logs/log_runs.csv" skip 1 using (jams($3)):(1) smooth cumulative \
        with lines lw 3 lc rgb "red" title "JAMS"

