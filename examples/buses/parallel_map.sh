#!/bin/bash

#$ -N buses_map
#$ -pe lam 8
#$ -l h_rt=00:10:00
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

mpirun -np $NSLOTS ../../bin/parallel -c "sh ./map.sh buses data %0 %1" -n 1 -f "map_%1_%0.flag" -p map_list.txt
#########################################################################
