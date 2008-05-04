#!/bin/sh

base=/state/partition2/obaltzer/$1
target=/state/partition2/obaltzer/vis/$1

alias visualize=$HOME/gm/bin/visualize
alias topk=$HOME/gm/bin/topk

collections=`ls ${base}`

if test ! -e ${target} ; then
    mkdir -p ${target}
fi

for c in ${collections} ; do
    data=${base}/${c}/data.dat
    find ${base}/${c}/ -name \*.grp | while read f ; do
        t=`echo $f | sed -e "s|${base}/${c}/||g" | sed -e 's/\//_/g' | sed -e 's/\.grp/\.top/g'`
        i=`echo $t | sed -e 's/\.top/\.png/g'`
#        echo topk -k 15 -o ${target}/$t $f 
        topk -k 15 -o ${target}/$t $f  >> /dev/null
#        echo visualize -l -g ${target}/$t -w 300 -h 300 -o ${target}/$i ${data}
        visualize -l -g ${target}/$t -w 300 -h 300 -o ${target}/$i ${data} >> /dev/null 2>&1
    done
done
echo done `hostname`
