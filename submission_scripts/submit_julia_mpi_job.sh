#!/bin/sh 

#SBATCH --time=00:10:00
#SBATCH --nodes=2
#SBATCH -A iccom013c
#SBATCH -p ProdQ

module load openmpi/intel/3.1.2

Start_time=$(date +%s)

mpiexec -n 2 julia --threads 4 ../min_width_sampling/MPI_example.jl >> tmp.out

End_time=$(date +%s)

echo "Time elapsed is $((End_time-Start_time))" >> tmp.out