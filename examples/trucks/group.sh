#!/bin/sh

. ../functions.sh

file=`chop_ext $1`
base_name=`echo ${file} | cut -d"_" -f1`
al=`echo ${file} | cut -d"_" -f2`

if test "${al}" == "cc" ; then
    support=`echo ${file} | cut -d"_" -f3`
    min_length=`echo ${file} | cut -d"_" -f4`
    time_level=`echo ${file} | cut -d"_" -f5`
    space_level=`echo ${file} | cut -d"_" -f6`
    do_cc
elif test "${al}" == "nc" ; then
    nc_threshold=`echo ${file} | cut -d"_" -f3`
    support=`echo ${file} | cut -d"_" -f4`
    min_length=`echo ${file} | cut -d"_" -f5`
    time_level=`echo ${file} | cut -d"_" -f6`
    space_level=`echo ${file} | cut -d"_" -f7`
    do_nc
fi
touch $1
