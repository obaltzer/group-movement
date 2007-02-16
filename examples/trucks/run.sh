#!/bin/sh

. ../functions.sh

base_name=trucks
time_level=2
support=40
min_length=5
width=400
height=400
nc_threshold=0.7

# convert dataset
if test ! -f ${base_name}.dat ; then
    ruby ${TOOLS_PATH}/convert.rb trucks.txt ${base_name}.dat
fi

# generate pretty pictures
reference_image
groups_reference_image

run -v -cc -nc
