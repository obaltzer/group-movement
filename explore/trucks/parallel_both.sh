#!/bin/bash

#$ -N trucks_both
#$ -pe lam 24
#$ -l h_rt=200:00:00
#$ -V
##$ -j yes
#$ -S /bin/bash
#$ -cwd
#$ -e logs
#$ -o logs

#########################################################################
#
# Run your parallel program here.

mpirun -np $NSLOTS ../../bin/parallel_batch -c "bash ../both.sh trucks.dat /state/partition2/obaltzer/trucks/%N %0 %1 %2 %3 %4 %5" -s 500 -n 1 -f "both_%5_%4_%3_%2_%1_%0.flag" -p both_list.txt
#########################################################################