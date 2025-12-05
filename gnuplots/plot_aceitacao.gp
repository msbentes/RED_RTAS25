set datafile separator ","
set terminal png size 900,600
set output "graficos/plot_aceitacao.png"

set title "Comparação de Aceitação: RED vs JAMS"
set style fill solid 1.0 border -1
set boxwidth 0.6
set grid ytics

# Coluna 2 = TarefasAceitasMedia
plot "logs/dados_comparativos.csv" using 2:xtic(1) title "Tarefas Aceitas" with boxes
