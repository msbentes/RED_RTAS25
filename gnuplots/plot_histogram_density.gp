set datafile separator ","

set title "Histograma Normalizado (Densidade) - RED vs JAMS"
set xlabel "Tempo (ms)"
set ylabel "Densidade"

bin_width = 5
bin(x,width) = width*floor(x/width)

# total de linhas por algoritmo
RED_N = real(system("awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv | wc -l"))
JAMS_N = real(system("awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv | wc -l"))

plot \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1.0/RED_N) smooth freq with boxes title "RED", \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1.0/JAMS_N) smooth freq with boxes title "JAMS"

