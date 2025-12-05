#!/bin/bash

input="logs/log_runs.csv"
output="logs/stats.csv"

echo "algorithm,mean_time,max_time,min_time,std_time" > "$output"

for alg in RED JAMS; do
    # extrai tempos apenas daquele algoritmo
    times=$(awk -F',' -v alg="$alg" '$2==alg {print $4}' "$input")

    # converte lista para array bash
    arr=($times)

    n=${#arr[@]}
    if (( n == 0 )); then
        echo "$alg,0,0,0,0" >> "$output"
        continue
    fi

    # média
    sum=0
    for v in "${arr[@]}"; do sum=$(echo "$sum + $v" | bc -l); done
    mean=$(echo "$sum / $n" | bc -l)

    # min/max
    min=${arr[0]}
    max=${arr[0]}
    for v in "${arr[@]}"; do
        (( $(echo "$v < $min" | bc -l) )) && min=$v
        (( $(echo "$v > $max" | bc -l) )) && max=$v
    done

    # desvio padrão
    var=0
    for v in "${arr[@]}"; do
        diff=$(echo "$v - $mean" | bc -l)
        sq=$(echo "$diff * $diff" | bc -l)
        var=$(echo "$var + $sq" | bc -l)
    done
    std=$(echo "sqrt($var / $n)" | bc -l)

    echo "$alg,$mean,$max,$min,$std" >> "$output"
done

