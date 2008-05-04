#!/bin/bash

host=`hostname`
tar -cz -C /state/partition2/obaltzer -f $1-${host}.tar.gz $1
