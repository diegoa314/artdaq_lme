#!/bin/bash

if [ $# -lt 2 ];then
	echo "USAGE: $0 <fcl_file> <n_procs>"
	exit 1
fi


fcl=$1
nprocs=$2

if ! [ -e $fcl ];then
	echo "File $fcl Not Found! Aborting..."
	exit 1
fi

if [ $nprocs -lt 2 ]; then
	echo "Must use at least 2 processes!"
	exit 2
fi

log=`basename $fcl|cut -f1 -d.`

# No concurrency of transfer_driver tests!
exec 200>/tmp/transfer_driver_`basename $fcl`.lockfile
flock -e 200

for ii in `seq $nprocs`;do
  rank=$(($ii - 1))
  transfer_driver $rank $fcl & PIDS[$ii]=$!
done

rc=0
for jj in ${PIDS[@]}; do
  wait $jj
  rc=$(($rc + $?))
done

exit $rc

