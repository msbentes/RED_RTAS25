set datafile separator ","

set title "Histograma - Tempo (ms) - RED vs JAMS"
set xlabel "Tempo (ms)"
set ylabel "Frequência"

bin_width = 5
bin(x,width) = width*floor(x/width)

plot \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1) smooth freq with boxes title "RED", \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1) smooth freq with boxes title "JAMS"

