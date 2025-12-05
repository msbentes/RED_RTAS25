set datafile separator ","

set multiplot layout 1,2 title "Histogramas - RED e JAMS"

set xlabel "Tempo (ms)"
set ylabel "Frequência"
set title "RED"
bin_width = 5
bin(x,width) = width*floor(x/width)
plot "<awk -F',' '$2==\"RED\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1) smooth freq with boxes notitle

set xlabel "Tempo (ms)"
set ylabel "Frequência"
set title "JAMS"
plot "<awk -F',' '$2==\"JAMS\"{print $4}' logs/log_runs.csv" using (bin($1,bin_width)):(1) smooth freq with boxes notitle

unset multiplot

