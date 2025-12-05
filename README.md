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
make

## Como executar
./run.sh

## Estrutura
- `/` → Códigos-fonte do simulador RED.
- `scripts/` → Scripts de execução e preparação do ambiente.
- `traces/` → Workloads reais ou sintéticos.
- `logs/` → Logs produzidos após cada execução.
- `gnuplots/` → Scripts para gerar gráficos.
- `graficos/` → Graficos produzidos após cada execução.


## ESQUEMA
```
RED_RTAS25/
│
├── README.md
├── LICENSE
│── sim.c
│── Makefile
││
├── traces/
│   ├── MPC_times/MPC_long_10/*.csv
│   └── MPC_times/MPC_short_10/*.csv
│
├── logs/
│   ├── dados_comparativos.csv
│   ├── log_runs.csv
│   └── log_tasks.csv
│
├── gnuplots/
│   ├── plot_tempo_medio.gp
│   ├── plot_aceitacao.gp
│   └── plot_comparativo.gp
│
└── graficos/
    ├── plot_aceitacao.png
    ├── plot_comparativo.png
    └── plot_tempo_medio.png
```

