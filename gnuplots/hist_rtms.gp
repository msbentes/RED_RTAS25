set terminal pngcairo size 1200,800
set output "hist_rtms.png"
set datafile separator ","

set title "Histograma RT_ms — RED vs JAMS"
set xlabel "rt_ms"
set ylabel "Frequência"

bin_width = 0.005
bin(x,width) = width * floor(x/width)

plot \
    "log_tasks.csv" using (bin($4,bin_width)):(1.0) every ::1::10 smooth freq with boxes lc rgb "blue" title "RED", \
    "log_tasks.csv" using (bin($4,bin_width)):(1.0) every ::500::510 smooth freq with boxes lc rgb "red" title "JAMS"

