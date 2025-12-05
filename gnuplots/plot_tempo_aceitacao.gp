# --- Configurações básicas ---
set terminal pngcairo size 800,600 enhanced font 'Arial,12'
set output "graficos/grafico_comparativo.png"

set datafile separator ","        # CSV usa vírgula
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.9

# --- Títulos e rótulos ---
set title "Comparativo RED vs JAMS"
set xlabel "Algoritmo"
set ylabel "Valores"
set grid ytics

# --- Legenda ---
set key outside

# --- Plot ---
# Usamos 'firstrow' para ignorar cabeçalho
plot "logs/dados_comparativos.csv" using 2:xtic(1) title "Tarefas Aceitas" , \
     "" using 3 title "Tempo Médio (ms)"
