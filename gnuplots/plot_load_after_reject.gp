set datafile separator ","
threshold = 0.8

# ---------- RED ----------
set terminal pngcairo size 1400,900
set output "load_after_RED_reject.png"

set title "Evolução da Carga — RED (marcando rejeições)"
set xlabel "task_id"
set ylabel "load_after_reject"
set grid

plot \
    "log_tasks.csv" using (strcol(2) eq "RED" ? $1 : 1/0):(strcol(2) eq "RED" ? $6 : 1/0) \
        with lines lw 2 lc rgb "blue" title "RED", \
    "log_tasks.csv" using (strcol(2) eq "RED" && $3==0 ? $1 : 1/0):(strcol(2) eq "RED" && $3==0 ? $6 : 1/0) \
        with points pt 7 ps 1.8 lc rgb "red" title "Rejeições", \
    threshold with lines dt 2 lw 3 lc rgb "black" title sprintf("Threshold = %.2f", threshold)

# ---------- JAMS ----------
set terminal pngcairo size 1400,900
set output "load_after_JAMS_refect.png"

set title "Evolução da Carga — JAMS (marcando rejeições)"
set xlabel "task_id"
set ylabel "load_after_reject"
set grid

plot \
    "log_tasks.csv" using (strcol(2) eq "JAMS" ? $1 : 1/0):(strcol(2) eq "JAMS" ? $6 : 1/0) \
        with lines lw 2 lc rgb "red" title "JAMS", \
    "log_tasks.csv" using (strcol(2) eq "JAMS" && $3==0 ? $1 : 1/0):(strcol(2) eq "JAMS" && $3==0 ? $6 : 1/0) \
        with points pt 7 ps 1.8 lc rgb "blue" title "Rejeições", \
    threshold with lines dt 2 lw 3 lc rgb "black" title sprintf("Threshold = %.2f", threshold)

