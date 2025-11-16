# RED_RTAS25 — Robust Earliest Deadline (Simulador)

Este repositório contém uma implementação simples e didática do
algoritmo RED (Robust Earliest Deadline) para fins de comparação com
JAMS (Job Acceptance Multiserver), no estilo do repositório oficial
do JAMS utilizado no artifact evaluation da RTAS 2025.

## Objetivos
- Reproduzir comportamento básico do RED.
- Simular cargas sintéticas ou reais.
- Permitir comparação com JAMS em Raspberry Pi (isolcpus).
- Gerar métricas: deadlines perdidos, latência, utilização, ocupação.

## Como compilar
cd src
make

## Como executar
./scripts/run_red.sh traces/example_small.csv

ou

taskset -c 3 src/red_sim traces/example_small.csv | tee "../results/logs/run_$(date +%s).log"

## Estrutura
- `src/` → Códigos-fonte do simulador RED.
- `scripts/` → Scripts de execução e preparação do ambiente.
- `traces/` → Workloads reais ou sintéticos.
- `analysis/` → Scripts Python para gerar gráficos.
- `results/` → Logs produzidos após cada execução.

## Compatível com Raspberry Pi
- Suporte a isolcpus.
- Script automático `pin_cpu.sh` para bind de afinidade.

## ESQUEMA
```
RED_RTAS25/
│
├── README.md
├── LICENSE
│
├── src/
│   ├── red.h
│   ├── red.c
│   ├── simulator.c
│   ├── parser.c
│   ├── parser.h
│   └── Makefile
│
├── scripts/
│   ├── setup_env.sh
│   ├── run_red.sh
│   ├── run_batch.sh
│   └── pin_cpu.sh
│
├── traces/
│   ├── example_small.csv
│   ├── synthetic_generator.py
│   └── README.md
│
├── results/
│   ├── logs/
│   ├── plots/
│   └── summary/
│
└── analysis/
    ├── plot_latency.py
    ├── plot_utilization.py
    └── plot_deadlines.py
```

