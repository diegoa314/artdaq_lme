# simple test configuration
# run on one node with 8 cores, configed as though it were 
# two nodes, one node being the detector layer and the other
# the builder layer.
# The builder layer has 3 sources and 2 sinks on it
# The detector layer has 3 event fragment producers on it.
# the total event size is 100000.
# the number of buffers allocated for send/receive ops is 10
# the number of events total to send is 10
# the run number is 1

# single node with no hostfile need with above parameters
# mpirun -n 8 ./builder 2 10 3 3 2 100000 10 1

# this uses the slot counting in the hostfile (openmpi)
# make a good configuration file using PBS_NODEFILE as input
./makeNodeFile.sh $PBS_NODEFILE 3 3 2 > /tmp/nfile

# for openmpi - run using generated hostfile
# must still tell the number of processes and the number of nodes
mpirun -hostfile /tmp/nfile -n 8 ./builder 2 10 3 3 2 100000 10 1

# mvapich needed one host per line in the hostfile
# for mvapich2
# mpirun_rsh -hostfile /tmp/nfile -np 8 ./builder 2 10 3 3 2 100000 10 1

# mvapich needed one host per line in the hostfile
# for mvapich
# mpirun_rsh_32 -ssh -hostfile /tmp/nfile -np 8 ./builder 2 10 3 3 2 100000 10 1
