set datafile separator ","

# ---------- PLOT RED ----------
set terminal pngcairo size 1400,900
set output "load_after_RED.png"

set title "Evolução da Carga (RED)"
set xlabel "task_id"
set ylabel "load_after"
set grid
threshold = 0.8

plot \
    "log_tasks.csv" using (strcol(2) eq "RED" ? $1 : 1/0):(strcol(2) eq "RED" ? $6 : 1/0) \
        with lines lw 2 lc rgb "blue" title "RED", \
    threshold with lines dt 2 lw 3 lc rgb "black" title sprintf("Threshold = %.2f", threshold)

# ---------- PLOT JAMS ----------
set terminal pngcairo size 1400,900
set output "load_after_JAMS.png"

set title "Evolução da Carga (JAMS)"
set xlabel "task_id"
set ylabel "load_after"
set grid

plot \
    "log_tasks.csv" using (strcol(2) eq "JAMS" ? $1 : 1/0):(strcol(2) eq "JAMS" ? $6 : 1/0) \
        with lines lw 2 lc rgb "red" title "JAMS", \
    threshold with lines dt 2 lw 3 lc rgb "black" title sprintf("Threshold = %.2f", threshold)

