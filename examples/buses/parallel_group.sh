#!/bin/bash

#$ -N buses_group
#$ -pe lam 48
#$ -l h_rt=08:00:00
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

mpirun -np $NSLOTS ../../bin/parallel -c "sh ./group.sh buses data %0 %1 %2 %3 %4 %5" -n 1 -f "group_%5_%4_%3_%2_%1_%0.flag" -p group_list.txt
#########################################################################
