TOOLS_PATH=../../tools
BIN_PATH=../../bin
FIM_APP=${BIN_PATH}/fim_max_rlimit
TIME=/usr/bin/time

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
    echo ${BIN_PATH}/levels ${base_name}.dat
    levels=`${BIN_PATH}/levels ${base_name}.dat`
    lx=`echo ${levels} | cut -d" " -f1`
    ly=`echo ${levels} | cut -d" " -f2`
    lt=`echo ${levels} | cut -d" " -f3`
    # check if the current configuration is feasible
    if test ${space_level} -le ${lx} -a ${time_level} -le ${lt} ; then
        l=`expr ${lx} - ${space_level}`
        t=`expr ${lt} - ${time_level}`
        filename="${time_level}_${space_level}"
        path="${target_dir}/${base_name}/map"
        mkdir -p ${path}
        stats="${path}/${filename}.stats"
        echo "map: ${filename}.emp"
        # write map file
        runtime=`get_time ${BIN_PATH}/map -l ${l},${l},${t} -o ${path}/${filename}.map ${base_name}.dat`
        echo "MAP Runtime: ${runtime}" > ${stats}
        # enumerate samples
        runtime=`get_time ${BIN_PATH}/enumerate -u ${path}/${filename}.unq -o ${path}/${filename}.emp ${path}/${filename}.map`
        echo "EMAP Runtime: ${runtime}" >> ${stats}
        trajectories=`wc -l ${path}/${filename}.emp | awk '{print $1}'`
        echo "Trajectories: ${trajectories}" >> ${stats}
        unique=`wc -l ${path}/${filename}.unq | awk '{print $1}'`
        echo "Unique Items: ${unique}" >> ${stats}
    fi
}
        
function fim()
{
    path="${target_dir}/${base_name}/fim/${support}"
    mkdir -p ${path}
    empname="${target_dir}/${base_name}/map/${time_level}_${space_level}.emp"
    finame="${path}/${time_level}_${space_level}.fi"
    stats="${path}/${time_level}_${space_level}.stats"
    if test -s "${empname}" ; then
        echo "fim: ${finame}"
        # run frequent itemset mining
        runtime=`get_time ${FIM_APP} ${empname} ${support} ${finame}.tmp`
        mv ${finame}.tmp ${finame}
        echo "FIM Runtime: ${runtime}" > ${stats}
        nfi=`wc -l ${finame} | awk '{print $1}'`
        echo "Frequent Itemsets: ${nfi}" >> ${stats}
    fi
}

function match()
{
    path="${target_dir}/${base_name}/cliques/${support}/${min_length}"
    mkdir -p ${path}
    empname="${target_dir}/${base_name}/map/${time_level}_${space_level}.emp"
    finame="${target_dir}/${base_name}/fim/${support}/${time_level}_${space_level}.fi"
    clname="${path}/${time_level}_${space_level}.cl"
    stats="${path}/${time_level}_${space_level}.stats"
    if test -s "${finame}" ; then
        echo "match: ${clname}"
        # match frequent items with trajectories (generate cliques)
        runtime=`get_time ${BIN_PATH}/match -l ${min_length} ${finame} ${empname} ${clname}`
        echo "MATCH Runtime: ${runtime}" > ${stats}
        ncl=`wc -l ${clname} | awk '{print $1}'`
        echo "Cliques: ${ncl}" >> ${stats}
    fi
}

function group()
{
    if test "${algorithm}" != "cc" ; then
        path="${target_dir}/${base_name}/${algorithm}/${weight}/${support}/${min_length}"
    else
        path="${target_dir}/${base_name}/${algorithm}/0.0/${support}/${min_length}"
    fi
    datname="${base_name}.dat"
    empname="${target_dir}/${base_name}/map/${time_level}_${space_level}.emp"
    finame="${target_dir}/${base_name}/fim/${support}/${time_level}_${space_level}.fi"
    clname="${target_dir}/${base_name}/cliques/${support}/${min_length}/${time_level}_${space_level}.cl"
    grpname="${path}/${time_level}_${space_level}.grp"
    stats="${path}/${time_level}_${space_level}.stats"
    if test -s "${clname}" ; then
        mkdir -p ${path}
        echo "${algorithm}: ${grpname}"
        if test "${algorithm}" == "cc" ; then
            runtime=`get_time ${BIN_PATH}/${algorithm} ${datname} ${clname} ${grpname}`
        elif test "${algorithm}" == "nc" ; then 
            runtime=`get_time ${BIN_PATH}/${algorithm} -t ${weight} ${datname} ${clname} ${grpname}`
        elif test "${algorithm}" == "nc2" ; then 
            runtime=`get_time ${BIN_PATH}/${algorithm} -t ${weight} -f ${finame} ${datname} ${clname} ${grpname}`
        elif test "${algorithm}" == "wc" -o "${algorithm}" == "wc2" ; then 
            runtime=`get_time ${BIN_PATH}/${algorithm} -t ${weight} -f ${finame} -e ${empname} ${datname} ${clname} ${grpname}`
        fi
        echo "${algorithm} Runtime: ${runtime}" > ${stats}
        ngrp=`wc -l ${grpname} | awk '{print $1}'`
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
