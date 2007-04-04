#!/bin/bash

#$ -N trucks_map
#$ -pe lam 8
#$ -l h_rt=00:10:00
#$ -V
##$ -j yes
#$ -S /bin/bash
#$ -cwd

#########################################################################
#
# Run your parallel program here.

echo "Number of slots is: $NSLOTS"

mpirun -np $NSLOTS ../../src/parallel -c "sh ./map.sh tmp/trucks_%1_%0.emp" -n 1 -f "map_%1_%0.flag" -p map_list.txt
#########################################################################
