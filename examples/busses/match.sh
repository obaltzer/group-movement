#!/bin/sh

. ../functions.sh

file=`chop_ext $1`
base_name=`echo ${file} | cut -d"_" -f1`
support=`echo ${file} | cut -d"_" -f2`
min_length=`echo ${file} | cut -d"_" -f3`
time_level=`echo ${file} | cut -d"_" -f4`
space_level=`echo ${file} | cut -d"_" -f5`

match
