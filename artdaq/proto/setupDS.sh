
# setup script for ds1.fnal.gov

# for mvapich1 setup
# source /usr/local/mvapich/etc/mvapich.conf.sh
# export PATH=/usr/local/mvapich/bin/:${PATH}

# for mvapich2 setup
# source /usr/local/mvapich2/etc/mvapich2.conf.sh
# export PATH=/usr/local/mvapich2/bin/:${PATH}

# for openmpi
export LD_LIBRARY_PATH=/usr/local/openmpi/lib/:${LD_LIBRARY_PATH}
export PATH=/usr/local/openmpi/bin/:${PATH}
export MANPATH=${MANPATH}:/usr/local/openmpi/share/man

# needed for all
export PATH=/usr/local/ruby-1.9.2-p136/bin:${PATH}

