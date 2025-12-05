set terminal pngcairo size 1600,600 enhanced font "Arial,16"
set output "graficos/hist_sidebyside.png"

set datafile separator ","

bin_width = 5
bin(x,width) = width*floor(x/width)

set multiplot layout 1,2 title "Histogramas - RED e JAMS" font ",20"

# --- RED ---
set title "RED"
set xlabel "Tempo (ms)"
set ylabel "Frequência"

plot "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" \
        using (bin($1,bin_width)):(1) smooth freq with boxes lc rgb "#1f77b4" notitle

# --- JAMS ---
set title "JAMS"
set xlabel "Tempo (ms)"
set ylabel "Frequência"

plot "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" \
        using (bin($1,bin_width)):(1) smooth freq with boxes lc rgb "#d62728" notitle

unset multiplot

