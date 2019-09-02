#!/bin/bash

# for openmpi runs
# must first source the setupDS.sh script

RUN=1
EVENTS=1000
BUFFERS=10
NODES=2
EVENT_SIZE=100000
DETS_PER=3
SRCS_PER=3
SINKS_PER=2

if [ -z "${PBS_NODEFILE}" ]
then
    echo -e "localhost\nlocalhost" > ./tmp_nodefile
    export PBS_NODEFILE="./tmp_nodefile"
fi

# generate a useful hostfile with fixed name
echo "filling on nodefile ${PBS_NODEFILE}"
NP=`./makeNodeFile.rb ${PBS_NODEFILE} ${DETS_PER} ${SRCS_PER} ${SINKS_PER} ${RUN}`

HOSTFILE="hostfilem_${RUN}.txt"
# HOSTFILE="hostfile_${RUN}.txt"

# run
ARGS="$NODES $EVENTS $DETS_PER $SRCS_PER $SINKS_PER $EVENT_SIZE $BUFFERS $RUN"
mpirun_rsh_1 -hostfile ${HOSTFILE} -np ${NP} ./builder ${ARGS}
# mpirun -hostfile ${HOSTFILE} -n ${NP} ./builder ${ARGS}

# round up performance files and configuration information
PFILES="perf_*${RUN}_*"
CFILES="config_*${RUN}_*"
PFILE_OUT="r_perf_${RUN}.txt"
CFILE_OUT="r_config_${RUN}.txt"

./cat.rb ${PFILE_OUT} ${PFILES}
cat ${CFILES} > ${CFILE_OUT}

rm -f ${PFILES}
rm -f ${CFILES}
