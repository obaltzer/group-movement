#!/bin/sh

. ../functions.sh

base_name=buses
time_level=2
support=5
min_length=40
width=400
height=400
nc_threshold=0.7

# convert dataset
if test ! -f ${base_name}.dat ; then
#    ruby ${TOOLS_PATH}/convert.rb --box 428305,4187012,501635,4238030 schoolbuses.txt ${base_name}.dat
    ruby ${TOOLS_PATH}/convert.rb --box 428305,4187012,495000,4220000 schoolbuses.txt ${base_name}.dat
fi

# generate pretty pictures
reference_image
groups_reference_image

run -v -cc -nc
