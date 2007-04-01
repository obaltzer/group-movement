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
elif test "${al}" == "wc" ; then
    wc_threshold=`echo ${file} | cut -d"_" -f3`
    support=`echo ${file} | cut -d"_" -f4`
    min_length=`echo ${file} | cut -d"_" -f5`
    time_level=`echo ${file} | cut -d"_" -f6`
    space_level=`echo ${file} | cut -d"_" -f7`
    do_wc
elif test "${al}" == "wc2" ; then
    wc_threshold=`echo ${file} | cut -d"_" -f3`
    support=`echo ${file} | cut -d"_" -f4`
    min_length=`echo ${file} | cut -d"_" -f5`
    time_level=`echo ${file} | cut -d"_" -f6`
    space_level=`echo ${file} | cut -d"_" -f7`
    do_wc2
elif test "${al}" == "nc2" ; then
    nc_threshold=`echo ${file} | cut -d"_" -f3`
    support=`echo ${file} | cut -d"_" -f4`
    min_length=`echo ${file} | cut -d"_" -f5`
    time_level=`echo ${file} | cut -d"_" -f6`
    space_level=`echo ${file} | cut -d"_" -f7`
    do_nc2
fi
touch $1
