#!/bin/bash
for t in ../traces/*.csv; do
    ./run_red.sh "$t"
done

