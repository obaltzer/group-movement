#!/bin/sh

base_name=random
levels=8
time_level=0
support=5
min_length=3
width=400
height=400

TOOLS_PATH=../../tools
BIN_PATH=../../bin

# generate dataset
ruby ${TOOLS_PATH}/datagen.rb ${base_name}.conf ${base_name}.dat

# generate pretty pictures
${BIN_PATH}/visualize -w ${width} -h ${height} -o ${base_name}_ref.png ${base_name}.dat
${BIN_PATH}/visualize -w ${width} -h ${height} -l -o ${base_name}_groups.png ${base_name}.dat

if test ! -d tmp ; then
    mkdir tmp
fi

level=0
# for each level remap the data set
while test ${level} -ne ${levels} ; do
    echo "Running level ${level}:"
    echo "======================="
    l=`expr ${levels} - ${level}`
    # write map file
    ${BIN_PATH}/map -l ${l},${l},${time_level} -o tmp/${base_name}_${level}.map ${base_name}.dat
    # enumerate samples
    ${BIN_PATH}/enumerate -u tmp/${base_name}_${level}.unq -o tmp/${base_name}_${level}.emp tmp/${base_name}_${level}.map
    # run frequent itemset mining
    ${BIN_PATH}/fim_max tmp/${base_name}_${level}.emp ${support} tmp/${base_name}_${level}.fi > /dev/null
    # match frequent items with trajectories (generate cliques)
    ${BIN_PATH}/match -l ${min_length} tmp/${base_name}_${level}.fi tmp/${base_name}_${level}.emp tmp/${base_name}_${level}.cl
    n_cl=`wc -l tmp/${base_name}_${level}.cl | awk '{print $1}'`
    if test ${n_cl} -gt 0 ; then
        # perform connected components grouping
        ${BIN_PATH}/cc ${base_name}.dat tmp/${base_name}_${level}.cl tmp/${base_name}_${level}.grp
        # make pretty picture
        ${BIN_PATH}/visualize -w ${width} -h ${height} -o ${base_name}_${level}.png -g tmp/${base_name}_${level}.grp ${base_name}.dat
    fi
    level=`expr ${level} + 1`
done
