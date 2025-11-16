#!/bin/bash
TRACE=$1
if [ -z "$TRACE" ]; then
    echo "Uso: run_red.sh arquivo.csv"
    exit 1
fi

DIR=$(dirname "$0")
BIN="../src/red_sim"

$DIR/pin_cpu.sh $BIN "$TRACE" | tee "../results/logs/run_$(date +%s).log"

