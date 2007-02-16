#!/bin/sh

. ../functions.sh

empfile=`chop_ext $1`
base_name=`echo ${empfile} | cut -d"_" -f1`
time_level=`echo ${empfile} | cut -d"_" -f2`
space_level=`echo ${empfile} | cut -d"_" -f3`

map
