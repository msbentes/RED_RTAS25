set datafile separator ","
set terminal pngcairo size 1600,900 enhanced font "Arial,12"
set output "graficos/grafico_load.png"

set title "Evolução da Carga por Tarefa (RED vs JAMS)"
set xlabel "task_id"
set ylabel "load_after"
set grid

MAX_CAPACITY = 50
set arrow from 0,MAX_CAPACITY to 100,MAX_CAPACITY nohead lc rgb "blue" lw 2
set label sprintf("Threshold = %d", MAX_CAPACITY) at 1,MAX_CAPACITY+2

set key outside top center horizontal box

# Campos do CSV:
#   1 = task_id
#   2 = algorithm (string)
#   3 = accepted (0/1)
#   4 = response_time
#   5 = load_before
#   6 = load_after

plot \
    "logs/log_tarefas.csv" using (stringcolumn(2) eq "RED" ? $1 : 1/0): \
                          (stringcolumn(2) eq "RED" ? $6 : 1/0) \
        with linespoints lt rgb "green" lw 2 pt 7 ps 1 title "RED - load_after",\
\
    "logs/log_tarefas.csv" using (stringcolumn(2) eq "RED" && $3==0 ? $1 : 1/0): \
                          (stringcolumn(2) eq "RED" && $3==0 ? $6 : 1/0) \
        with points pt 7 ps 1 lc rgb "red" title "RED rejeitada",\
\
    "logs/log_tarefas.csv" using (stringcolumn(2) eq "JAMS" ? $1 : 1/0): \
                          (stringcolumn(2) eq "JAMS" ? $6 : 1/0) \
        with linespoints lt rgb "orange" lw 2 pt 5 ps 1 title "JAMS - load_after",\
\
    "logs/log_tarefas.csv" using (stringcolumn(2) eq "JAMS" && $3==0 ? $1 : 1/0): \
                          (stringcolumn(2) eq "JAMS" && $3==0 ? $6 : 1/0) \
        with points pt 7 ps 1 lc rgb "red" title "JAMS rejeitada"

unset output
print "Gerado: graficos/grafico_load.png"
