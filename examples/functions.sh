TOOLS_PATH=../../tools
BIN_PATH=../../bin
FIM_APP=${BIN_PATH}/fim_max_rlimit
TIME=/usr/local/bin/time

function reference_image()
{
    echo -n "Drawing reference image..."
    ${BIN_PATH}/visualize -w ${width} -h ${height} -o ${base_name}_ref.png ${base_name}.dat
    echo "done"
}

function groups_reference_image()
{
    echo -n "Drawing groups reference image..."
    ${BIN_PATH}/visualize -w ${width} -h ${height} -l -o ${base_name}_groups.png ${base_name}.dat
    echo "done"
}
    
function run()
{
    while test "$1" ; do
        case "$1" in
            -v)
                visualize=1
                ;;
            -wc)
                run_wc=1
                ;;
            -nc)
                run_nc=1
                ;;
            -cc)
                run_cc=1
        esac
        shift
    done

    if test ! -d tmp ; then
        mkdir tmp
    fi
    
    level=0
    levels=`${BIN_PATH}/levels ${base_name}.dat`
    lx=`echo ${levels} | cut -d" " -f1`
    ly=`echo ${levels} | cut -d" " -f2`
    lt=`echo ${levels} | cut -d" " -f3`
    # for each level remap the data set
    while test ${level} -ne ${lx} ; do
        l=`expr ${lx} - ${level}`
        t=`expr ${lt} - ${time_level}`
        echo 
        echo "Running level ${level},${level},${time_level}:"
        # write map file
        ${BIN_PATH}/map -l ${l},${l},${t} -o tmp/${base_name}_${level}.map ${base_name}.dat
        # enumerate samples
        ${BIN_PATH}/enumerate -u tmp/${base_name}_${level}.unq -o tmp/${base_name}_${level}.emp tmp/${base_name}_${level}.map > /dev/null
        # run frequent itemset mining
        ${FIM_APP} tmp/${base_name}_${level}.emp ${support} tmp/${base_name}_${level}.fi > /dev/null
        n_fi=`wc -l tmp/${base_name}_${level}.fi | awk '{print $1}'`
        echo "    Found itemsets: ${n_fi}"
        # match frequent items with trajectories (generate cliques)
        ${BIN_PATH}/match -l ${min_length} tmp/${base_name}_${level}.fi tmp/${base_name}_${level}.emp tmp/${base_name}_${level}.cl
        n_cl=`wc -l tmp/${base_name}_${level}.cl | awk '{print $1}'`
        echo "    Candidate cliques: ${n_cl}"
        if test ${n_cl} -gt 0 ; then
            if test "${run_cc}" ; then
                run_cc
            fi
            if test "${run_nc}" ; then
                run_nc
            fi
            if test "${run_wc}" ; then
                run_wc
            fi
        fi
        level=`expr ${level} + 1`
    done 
}

function run_cc()
{
    echo -n "    Performing cc... "
    ${BIN_PATH}/cc ${base_name}.dat tmp/${base_name}_${level}.cl tmp/${base_name}_${level}_cc.grp > /dev/null
    n_cc_grp=`wc -l tmp/${base_name}_${level}_cc.grp | awk '{print $1}'`
    echo "${n_cc_grp} groups"
    if test "${visualize}" ; then
        ${BIN_PATH}/visualize -w ${width} -h ${height} -o `printf ${base_name}_cc_%.2f_%02d.png 1.0 ${level}` -g tmp/${base_name}_${level}_cc.grp ${base_name}.dat
    fi
}

function run_nc()
{
    if test "${nc_threshold}" ; then
        echo -n "    Performing nc... "
        ${BIN_PATH}/nc -t ${nc_threshold} ${base_name}.dat tmp/${base_name}_${level}.cl tmp/${base_name}_${level}_nc_${nc_threshold}.grp > /dev/null
        n_nc_grp=`wc -l tmp/${base_name}_${level}_nc_${nc_threshold}.grp | awk '{print $1}'`
        echo "${n_nc_grp} groups"
        if test "${visualize}" ; then
            ${BIN_PATH}/visualize -w ${width} -h ${height} -o `printf ${base_name}_nc_%.2f_%02d.png ${nc_threshold} ${level}` -g tmp/${base_name}_${level}_nc_${nc_threshold}.grp ${base_name}.dat
        fi
    else
        echo "Cannot perform NC merging: nc_threshold value is missing."
    fi
}

function run_wc()
{
    if test "${wc_threshold}" ; then
        echo -n "    Performing wc... "
        ${BIN_PATH}/wc -t ${wc_threshold} -f tmp/${base_name}_${level}.fi -e tmp/${base_name}_${level}.emp ${base_name}.dat tmp/${base_name}_${level}.cl tmp/${base_name}_${level}_wc_${wc_threshold}.grp > /dev/null
        n_wc_grp=`wc -l tmp/${base_name}_${level}_wc_${wc_threshold}.grp | awk '{print $1}'`
        echo "${n_wc_grp} groups"
        if test "${visualize}" ; then
            ${BIN_PATH}/visualize -w ${width} -h ${height} -o `printf ${base_name}_wc_%.2f_%02d.png ${wc_threshold} ${level}` -g tmp/${base_name}_${level}_wc_${wc_threshold}.grp ${base_name}.dat
        fi
    else
        echo "Cannot perform WC merging: nc_threshold value is missing."
    fi
}

function map()
{
    levels=`${BIN_PATH}/levels ${base_name}.dat`
    lx=`echo ${levels} | cut -d" " -f1`
    ly=`echo ${levels} | cut -d" " -f2`
    lt=`echo ${levels} | cut -d" " -f3`
    # check if the current configuration is feasible
    if test ${space_level} -le ${lx} -a ${time_level} -le ${lt} ; then
        l=`expr ${lx} - ${space_level}`
        t=`expr ${lt} - ${time_level}`
        filename="${base_name}_${time_level}_${space_level}"
        stats="tmp/${base_name}_${time_level}_${space_level}.map_stats"
        echo "map: ${filename}.emp"
        # write map file
        runtime=`get_time ${BIN_PATH}/map -l ${l},${l},${t} -o tmp/${filename}.map ${base_name}.dat`
        echo "MAP Runtime: ${runtime}" > ${stats}
        # enumerate samples
        runtime=`get_time ${BIN_PATH}/enumerate -u tmp/${filename}.unq -o tmp/${filename}.emp tmp/${filename}.map`
        echo "EMAP Runtime: ${runtime}" >> ${stats}
        trajectories=`wc -l tmp/${filename}.emp | awk '{print $1}'`
        echo "Trajectories: ${trajectories}" >> ${stats}
        unique=`wc -l tmp/${filename}.unq | awk '{print $1}'`
        echo "Unique Items: ${unique}" >> ${stats}
    fi
}
        
function fim()
{
    empname="${base_name}_${time_level}_${space_level}.emp"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    stats="tmp/${base_name}_${support}_${time_level}_${space_level}.fi_stats"
    if test -s "tmp/${empname}" ; then
        echo "fim: ${finame}"
        # run frequent itemset mining
        runtime=`get_time ${FIM_APP} tmp/${empname} ${support} tmp/${finame}.tmp`
        mv tmp/${finame}.tmp tmp/${finame}
        echo "FIM Runtime: ${runtime}" > ${stats}
        nfi=`wc -l tmp/${finame} | awk '{print $1}'`
        echo "Frequent Itemsets: ${nfi}" >> ${stats}
    fi
}

function match()
{
    empname="${base_name}_${time_level}_${space_level}.emp"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    stats="tmp/${base_name}_${support}_${min_length}_${time_level}_${space_level}.match_stats"
    if test -s "tmp/${finame}" ; then
        echo "match: ${clname}"
        # match frequent items with trajectories (generate cliques)
        runtime=`get_time ${BIN_PATH}/match -l ${min_length} tmp/${finame} tmp/${empname} tmp/${clname}`
        echo "MATCH Runtime: ${runtime}" > ${stats}
        ncl=`wc -l tmp/${clname} | awk '{print $1}'`
        echo "Cliques: ${ncl}" >> ${stats}
    fi
}

function do_cc()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    grpname="${base_name}_cc_${support}_${min_length}_${time_level}_${space_level}.grp"
    stats="tmp/${base_name}_cc_${support}_${min_length}_${time_level}_${space_level}.stats"
    if test -s "tmp/${clname}" ; then
        echo "cc: ${grpname}"
        runtime=`get_time ${BIN_PATH}/cc ${datname} tmp/${clname} tmp/${grpname}`
        echo "CC Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l tmp/${grpname} | awk '{print $1}'`
        echo "Groups found: ${ngrp}" >> ${stats}
    fi
}

function do_nc()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    grpname="${base_name}_nc_${nc_threshold}_${support}_${min_length}_${time_level}_${space_level}.grp"
    stats="tmp/${base_name}_nc_${nc_threshold}_${support}_${min_length}_${time_level}_${space_level}.stats"
    if test -s "tmp/${clname}" ; then
        echo "nc: ${grpname}"
        runtime=`get_time ${BIN_PATH}/nc -t ${nc_threshold} ${datname} tmp/${clname} tmp/${grpname}`
        echo "NC Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l tmp/${grpname} | awk '{print $1}'`
        echo "Groups found: ${ngrp}" >> ${stats}
    fi
}

function do_nc2()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    grpname="${base_name}_nc2_${nc_threshold}_${support}_${min_length}_${time_level}_${space_level}.grp"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    stats="tmp/${base_name}_nc2_${nc_threshold}_${support}_${min_length}_${time_level}_${space_level}.stats"
    if test -s "tmp/${clname}" ; then
        echo "nc2: ${grpname}"
        runtime=`get_time ${BIN_PATH}/nc2 -t ${nc_threshold} -f tmp/${finame} ${datname} tmp/${clname} tmp/${grpname}`
        echo "NC2 Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l tmp/${grpname} | awk '{print $1}'`
        echo "Groups found: ${ngrp}" >> ${stats}
    fi
}

function do_wc()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    empname="${base_name}_${time_level}_${space_level}.emp"
    grpname="${base_name}_wc_${wc_threshold}_${support}_${min_length}_${time_level}_${space_level}.grp"
    stats="tmp/${base_name}_wc_${wc_threshold}_${support}_${min_length}_${time_level}_${space_level}.stats"
    if test -s "tmp/${clname}" ; then
        echo "wc: ${grpname}"
        runtime=`get_time ${BIN_PATH}/wc -t ${wc_threshold} -f tmp/${finame} -e tmp/${empname} ${datname} tmp/${clname} tmp/${grpname}`
        echo "WC Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l tmp/${grpname} | awk '{print $1}'`
        echo "Groups found: ${ngrp}" >> ${stats}
    fi
}

function do_wc2()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    empname="${base_name}_${time_level}_${space_level}.emp"
    grpname="${base_name}_wc2_${wc_threshold}_${support}_${min_length}_${time_level}_${space_level}.grp"
    stats="tmp/${base_name}_wc2_${wc_threshold}_${support}_${min_length}_${time_level}_${space_level}.stats"
    if test -s "tmp/${clname}" ; then
        echo "wc2: ${grpname}"
        runtime=`get_time ${BIN_PATH}/wc2 -t ${wc_threshold} -f tmp/${finame} -e tmp/${empname} ${datname} tmp/${clname} tmp/${grpname}`
        echo "WC2 Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l tmp/${grpname} | awk '{print $1}'`
        echo "Groups found: ${ngrp}" >> ${stats}
    fi
}

function get_time
{
    ${TIME} -f "%U" $@ 2>&1 1>/dev/null
}

function chop_ext
{
    basename $1 | sed -e 's/\.[[:alnum:]]\{1,\}$//'
}
