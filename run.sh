mkdir results/logs/
taskset -c 3 src/red_sim traces/example_small.csv | tee "results/logs/run_$(date +%s).log"

