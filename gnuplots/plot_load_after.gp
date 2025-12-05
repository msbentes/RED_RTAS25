set terminal pngcairo size 1400,900
set output "load_after.png"
set datafile separator ","

set title "Evolução da Carga (load_after × task_id)"
set xlabel "task_id"
set ylabel "load_after"
set grid

plot \
    "log_tasks.csv" using 1:6 with lines lw 2 lc rgb "blue" title "RED", \
    "log_tasks.csv" using 1:6 every ::500::10000 with lines lw 2 lc rgb "red" title "JAMS"

