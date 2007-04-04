#!/bin/bash

#$ -N trucks_match
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

mpirun -np $NSLOTS ../../src/parallel -c "sh ./match.sh tmp/trucks_%2_%3_%1_%0.cl" -n 1 -f "match_%2_%1_%0.flag" -p match_list.txt
#########################################################################
