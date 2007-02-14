TOOLS_PATH=../../tools
BIN_PATH=../../bin
FIM_APP=${BIN_PATH}/fim_max_rlimit

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
        ${BIN_PATH}/wc -t ${wc_threshold} ${base_name}.dat tmp/${base_name}_${level}.cl tmp/${base_name}_${level}_wc_${wc_threshold}.grp > /dev/null
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
        echo "map: ${filename}.emp"
        # write map file
        ${BIN_PATH}/map -l ${l},${l},${t} -o tmp/${filename}.map ${base_name}.dat
        # enumerate samples
        ${BIN_PATH}/enumerate -u tmp/${filename}.unq -o tmp/${filename}.emp tmp/${filename}.map > /dev/null
    fi
}
        
function fim()
{
    empname="${base_name}_${time_level}_${space_level}.emp"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    if test -s "tmp/${empname}" ; then
        echo "fim: ${finame}"
        # run frequent itemset mining
        ${FIM_APP} tmp/${empname} ${support} tmp/${finame} > /dev/null
    fi
}

function match()
{
    empname="${base_name}_${time_level}_${space_level}.emp"
    finame="${base_name}_${support}_${time_level}_${space_level}.fi"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    if test -s "tmp/${finame}" ; then
        echo "match: ${clname}"
        # match frequent items with trajectories (generate cliques)
        ${BIN_PATH}/match -l ${min_length} tmp/${finame} tmp/${empname} tmp/${clname}
    fi
}

function do_cc()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    grpname="${base_name}_cc_${support}_${min_length}_${time_level}_${space_level}.grp"
    if test -s "tmp/${clname}" ; then
        echo "cc: ${grpname}"
        ${BIN_PATH}/cc ${datname} tmp/${clname} tmp/${grpname} > /dev/null
    fi
}

function do_nc()
{
    datname="${base_name}.dat"
    clname="${base_name}_${support}_${min_length}_${time_level}_${space_level}.cl"
    grpname="${base_name}_nc_${nc_threshold}_${support}_${min_length}_${time_level}_${space_level}.grp"
    if test -s "tmp/${clname}" ; then
        echo "nc: ${grpname}"
        ${BIN_PATH}/nc -t ${nc_threshold} ${datname} tmp/${clname} tmp/${grpname} > /dev/null
    fi
}

function chop_ext
{
    basename $1 | sed -e 's/\.[^\.]\+$//'
}
