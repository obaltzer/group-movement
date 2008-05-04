#!/bin/sh

base=/state/partition2/obaltzer/vis
hostname=`hostname`
target=$HOME/$1-${hostname}.tar.gz

if test -e ${base}/$1 ; then
    tar -C ${base} -cz -f ${target} $1
fi
echo done `hostname`
