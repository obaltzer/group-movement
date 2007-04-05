#!/bin/bash

#$ -N buses_fim
#$ -pe lam 32
#$ -l h_rt=04:00:00
#$ -V
##$ -j yes
#$ -S /bin/bash
#$ -cwd
#$ -e logs
#$ -o logs
#########################################################################
#
# Run your parallel program here.

echo "Number of slots is: $NSLOTS"

mpirun -np $NSLOTS ../../bin/parallel -c "sh ./fim.sh buses data %0 %1 %2" -n 1 -f "fim_%2_%1_%0.flag" -p fim_list.txt
#########################################################################
