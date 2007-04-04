#!/bin/bash

#$ -N trucks_fim
#$ -pe lam 24
#$ -l h_rt=04:00:00
#$ -V
##$ -j yes
#$ -S /bin/bash
#$ -cwd

#########################################################################
#
# Run your parallel program here.

echo "Number of slots is: $NSLOTS"

mpirun -np $NSLOTS ../../src/parallel -c "sh ./fim.sh tmp/trucks_%2_%1_%0.fi" -n 1 -f "fim_%2_%1_%0.flag" -p fim_list.txt
#########################################################################
