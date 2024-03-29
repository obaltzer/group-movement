#!/bin/bash

#$ -N buses_match
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

mpirun -np $NSLOTS ../../bin/parallel -c "sh ./match.sh buses data %0 %1 %2 %3" -n 1 -f "match_%3_%2_%1_%0.flag" -p match_list.txt
#########################################################################
