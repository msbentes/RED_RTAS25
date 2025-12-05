set datafile separator ","
set term pngcairo size 1280,720 font "Arial,14"
set output "graficos/cdf_tempo.png"

set title "CDF do Tempo de Resposta — RED vs JAMS"
set xlabel "Tempo (ms)"
set ylabel "F(t)"
set grid

#
# Funções para filtrar cada algoritmo
#
red(x)  = (strcol(2) eq "RED")  ? x : NaN
jams(x) = (strcol(2) eq "JAMS") ? x : NaN

#
# Geração da CDF usando 'smooth cumulative'
#
plot \
    "logs/log_runs.csv" skip 1 using (red($4)):(1) smooth cumulative \
        with lines lw 3 lc rgb "blue" title "RED", \
    "logs/log_runs.csv" skip 1 using (jams($4)):(1) smooth cumulative \
        with lines lw 3 lc rgb "red" title "JAMS"

