#!/bin/bash

#$ -N random_25_overlap
#$ -pe lam 16
#$ -l h_rt=200:00:00
#$ -V
##$ -j yes
#$ -S /bin/bash
#$ -cwd
#$ -e logs
#$ -o logs

PWD=`pwd`
name=`basename ${PWD}`

#########################################################################
#
# Run your parallel program here.

mpirun -np $NSLOTS ../../bin/parallel_batch -c "bash ../overlap.sh output.dat /state/partition2/obaltzer/${name}/%N %0 %1 %2 %3 %4" -s 500 -n 1 -f "overlap_%4_%3_%2_%1_%0.flag" -p overlap_list.txt
#########################################################################
