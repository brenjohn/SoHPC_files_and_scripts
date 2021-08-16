#!/bin/sh 

#SBATCH --time=00:10:00
#SBATCH --nodes=1
#SBATCH -A iccom013c
#SBATCH -p ProdQ

Start_time=$(date +%s)

julia --project=. src/my_julia_script.jl >> tmp.out

End_time=$(date +%s)

echo "Time elapsed is $((End_time-Start_time))" >> tmp.out