#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
ENDC='\033[0m'

workloads=( workloads/workloada workloads/workloadb workloads/workloadc workloads/workloadd workloads/workloade workloads/workloadf )
data_dir=/mnt/h/dd
ycsb_program=./bin/ycsb
output_folder=output
num_threads=32
repetition=3

mkdir -p "$output_folder"
printf "${RED}Loading dataset${ENDC}\n"
"$ycsb_program" load rocksdb -s -P "$workload" -p rocksdb.dir="$data_dir" -threads 100 > "$output_folder/load.log" 2>&1

for workload in "${workloads[@]}"; do
	output_log_name=$(sed -r "s/.*\/(\w+)/\1/g" <<< "$workload")
	for iter in $(seq 1 $repetition); do
		printf "${CYAN}Operating on %s @ iter %d${ENDC}\n" "$workload" "$iter"
		"$ycsb_program" run rocksdb -s -P "$workload" -p rocksdb.dir="$data_dir" -threads "$num_threads" > "$output_folder/$output_log_name.iter$iter.log" 2>&1
	done
done
