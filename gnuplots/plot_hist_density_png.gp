set terminal pngcairo size 1200,800 enhanced font "Arial,16"
set output "graficos/hist_density.png"

set datafile separator ","

set title "Histograma Normalizado (Densidade) - RED vs JAMS"
set xlabel "Tempo (ms)"
set ylabel "Densidade"

bin_width = 5
bin(x,width) = width*floor(x/width)

# Total de linhas
RED_N  = real(system("awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv | wc -l"))
JAMS_N = real(system("awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv | wc -l"))

plot \
    "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" \
        using (bin($1,bin_width)):(1.0/RED_N) smooth freq with boxes lc rgb "#1f77b4" title "RED", \
    "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" \
        using (bin($1,bin_width)):(1.0/JAMS_N) smooth freq with boxes lc rgb "#d62728" title "JAMS"

