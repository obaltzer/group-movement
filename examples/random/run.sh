#!/bin/sh

. ../functions.sh

base_name=random
time_level=0
support=5
min_length=3
width=800
height=800
nc_threshold=0.25
wc_threshold=0.25

# generate dataset
ruby ${TOOLS_PATH}/datagen.rb ${base_name}.conf ${base_name}.dat

# generate pretty pictures
reference_image
groups_reference_image

run -v -cc -nc -wc
