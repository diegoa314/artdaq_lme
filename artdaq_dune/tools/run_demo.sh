#!/bin/bash

# JCF, Oct-5-2017
# This script basically follows the instructions found in https://cdcvs.fnal.gov/redmine/projects/artdaq-utilities/wiki/Artdaq-daqinterface

get_this_dir() 
{
    reldir=`dirname ${0}`
    ssi_mdt_dir=`cd ${reldir} && pwd -P`
}

validate_basedir()
{
	valid_basedir=0
	if [ -d $basedir/artdaq-utilities-daqinterface ] || [ -d $ARTDAQ_DAQINTERFACE_DIR ]; then
		if [ -d $basedir/DAQInterface ]; then
			valid_basedir=1
		fi
	fi
}

validate_toolsdir()
{
	valid_toolsdir=0
	if [ -f $toolsdir/xt_cmd.sh ]; then
		valid_toolsdir=1
	fi
}

validate_fhicldir()
{
	valid_fhicldir=0
	if [ -f $fhicldir/TransferInputShmem.fcl ]; then
		valid_fhicldir=1
	fi
}

get_this_dir
basedir=$ssi_mdt_dir
validate_basedir

toolsdir="$basedir/srcs/artdaq_demo/tools"
validate_toolsdir

fhicldir="$toolsdir/fcl"
validate_fhicldir


om_fhicl=TransferInputShmem

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options] [just_do_it.sh options]
examples: `basename $0` 
          `basename $0` --om --om_fhicl TransferInputShmemWithDelay
		  `basename $0` --om --config demo_largesystem --compfile $PWD/DAQInterface/comps.list --runduration 40
--help        This help message
--just_do_it_help Help message from just_do_it.sh
--basedir	  Base directory ($basedir, valid=$valid_basedir)
--toolsdir	  artdaq_demo/tools directory ($toolsdir, valid=$valid_toolsdir)
--fhicldir    Directory where Online Monitor FHiCL files reside (TransferInputShmem.fcl, etc) ($fhicldir, valid=$valid_fhicldir)
--no_om       Do *NOT* run Online Monitoring
--om_fhicl    Name of Fhicl file to use for online monitoring ($om_fhicl)
--partition=<N> set a partition number -- to allow multiple demos
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= do_jdi_help= do_om=1;
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
            \?*|h*)     eval $op1chr; do_help=1;;
            -help)      eval $op1arg; do_help=1;;
	    -just_do_it_help) eval $op1arg; do_jdi_help=1;;
            -basedir)   eval $reqarg; basedir=$1; shift;;
            -toolsdir)  eval $reqarg; toolsdir=$1; shift;;
	    -no_om)        do_om=0;;
	    -om_fhicl)  eval $reqarg; om_fhicl=$1; shift;;
            -partition) eval $reqarg; export ARTDAQ_PARTITION_NUMBER=$1; shift;;
            *)          aa=`echo "-$op" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'";
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa

test -n "${do_help-}" && echo "$USAGE" && exit
#echo "Remaining args: $@"


validate_basedir
validate_toolsdir
validate_fhicldir

if [ $valid_basedir -eq 0 ]; then
	echo "Provided base directory (${basedir}) is not valid! Must contain DAQInterface directory, and artdaq-utilities-daqinterface directory if \$ARTDAQ_DAQINTERFACE_DIR is not set"
	return 1
	exit 1
fi

toolsdir_save=$toolsdir
if [ $valid_toolsdir -eq 0 ]; then
	toolsdir="$basedir/srcs/artdaq_demo/tools"
	validate_toolsdir
	if [ $valid_toolsdir -eq 0 ]; then
		toolsdir="$ARTDAQ_DEMO_FQ_DIR/bin"
		validate_toolsdir
		
		if [ $valid_toolsdir -eq 0 ]; then
			echo "Provided tools directory (${toolsdir_save}) is not valid!"
			return 2
			exit 2
		fi
	fi
fi

fhicldir_save=$fhicldir
if [ $valid_fhicldir -eq 0 ]; then
	fhicldir="$basedir/srcs/artdaq_demo/tools/fcl"
	validate_fhicldir
	if [ $valid_fhicldir -eq 0 ]; then
		fhicldir="$ARTDAQ_DEMO_DIR/fcl"
		validate_fhicldir
		
		if [ $valid_fhicldir -eq 0 ]; then
			echo "Provided FHiCL directory (${fhicldir_save}) is not valid!"
			return 2
			exit 2
		fi
	fi
fi

daqintdir=$basedir/DAQInterface
jdibootfile=$daqintdir/boot.txt
jdiduration=200
cd $basedir


if [ -n "${do_jdi_help-}" ]; then
    cd ${daqintdir}
    source ./mock_ups_setup.sh	
	export DAQINTERFACE_USER_SOURCEFILE=$PWD/user_sourcefile_example
	source $ARTDAQ_DAQINTERFACE_DIR/source_me
	just_do_it.sh --help
	exit
fi


function wait_for_state() {
    local stateName=$1

    # 20-Mar-2018, KAB
    # The DAQInterface setup uses a dynamic way to determine which PORT to use for communication.
    # This means that we need to allow sufficient time between when DAQInterface is started and
    # when we run 'source_me' so that they agree on the port that should be used.
    # The way that we work around this here is to catch what appears to be a port mis-match
    # (the status.sh call returns an empty string), wait a bit, and then re-run source_me in the
    # hope that it will pick up the correct port.
    # An important piece of this is the un-setting of the DAQINTERFACE_STANDARD_SOURCEFILE_SOURCED
    # env var so that source_me will go through the process of re-determining which port to use.

    cd ${daqintdir}
    source ./mock_ups_setup.sh
    export DAQINTERFACE_USER_SOURCEFILE=$PWD/user_sourcefile_example
    source $ARTDAQ_DAQINTERFACE_DIR/source_me > /dev/null

    while [[ "1" ]]; do
      sleep 1

      # 19-Apr-2018, KAB: removed the redirection of stderr for the status.sh call
      # so that we will see problems like 'unable to find installed package' when
      # we install the demo on one node in a cluster and try to run it on another
      # node that has a different set of external disks mounted.
      res=$( status.sh | tail -1 | tr "'" " " | awk '{print $2}' )

      if [[ "$res" == "" ]]; then
          sleep 2
          unset DAQINTERFACE_STANDARD_SOURCEFILE_SOURCED
          source $ARTDAQ_DAQINTERFACE_DIR/source_me > /dev/null
      fi

      if [[ "$res" == "$stateName" ]]; then
          break
      fi
    done
}

function get_dispatcher_port() {
    
    cd ${daqintdir}
    source ./mock_ups_setup.sh
    export DAQINTERFACE_USER_SOURCEFILE=$PWD/user_sourcefile_example
    source $ARTDAQ_DAQINTERFACE_DIR/source_me > /dev/null

    source $ARTDAQ_DAQINTERFACE_DIR/bin/diagnostic_tools.sh

    thisdir=`ls -t $recorddir|head -1`
    #echo "Reading $recorddir/$thisdir/ranks.txt"
	dispatcherPort=`grep -i dispatcher $recorddir/$thisdir/ranks.txt|head -1|awk '{print $2}'`

	echo "Dispatcher found at port $dispatcherPort"

	return $dispatcherPort
}

# And now, actually run DAQInterface as described in
# https://cdcvs.fnal.gov/redmine/projects/artdaq-utilities/wiki/Artdaq-daqinterface

    $toolsdir/xt_cmd.sh $daqintdir --geom '132x33 -sl 2500' \
        -c 'source mock_ups_setup.sh' \
	-c 'export DAQINTERFACE_USER_SOURCEFILE=$PWD/user_sourcefile_example' \
	-c 'source $ARTDAQ_DAQINTERFACE_DIR/source_me' \
	-c 'DAQInterface'

    sleep 3
    echo ""
    echo "Waiting for DAQInterface to reached the 'stopped' state before continuing..."
    wait_for_state "stopped"
    echo "Done waiting."

    $toolsdir/xt_cmd.sh $daqintdir --geom 132 \
        -c 'source mock_ups_setup.sh' \
	-c 'export DAQINTERFACE_USER_SOURCEFILE=$PWD/user_sourcefile_example' \
	-c 'source $ARTDAQ_DAQINTERFACE_DIR/source_me' \
	-c 'if [[ -n $DAQINTERFACE_MESSAGEFACILITY_FHICL ]]; then msgfacfile=$DAQINTERFACE_MESSAGEFACILITY_FHICL ; else msgfacfile=MessageFacility.fcl ; fi' \
	-c 'if [[ -e $msgfacfile ]]; then sed -r -i  "s/(host\s*:\s*)\"\S+\"/\1\""$HOSTNAME"\"/g" $msgfacfile ; fi' \
	-c "just_do_it.sh -v $* $jdibootfile $jdiduration"

	if [ $do_om -eq 1 ]; then
    sleep 8;
    echo ""
    echo "Waiting for the run to start before starting online monitor apps..."
    wait_for_state "running"
    echo "Done waiting."

    get_dispatcher_port

	if [[ "x$dispatcherPort" != "x" ]]; then
    sed -r -i "s/dispatcherPort:.*/dispatcherPort: ${dispatcherPort}/" ${fhicldir}/${om_fhicl}.fcl

    xrdbproc=$( which xrdb )

    xloc=
    if [[ -e $xrdbproc ]]; then
    	xloc=$( xrdb -symbols | grep DWIDTH | awk 'BEGIN {FS="="} {pixels = $NF; print pixels/2}' )
    else
    	xloc=800
    fi

    $toolsdir/xt_cmd.sh $basedir --geom '150x33+'$xloc'+0 -sl 2500' \
        -c '. ./setupARTDAQDEMO' \
        -c 'art -c '$fhicldir'/'$om_fhicl'.fcl'

    sleep 4;

    $toolsdir/xt_cmd.sh $basedir --geom '100x33+0+0 -sl 2500' \
        -c '. ./setupARTDAQDEMO' \
    	-c 'rm -f /tmp/'$om_fhicl'2.fcl' \
        -c 'cp -p '$fhicldir'/'$om_fhicl'.fcl /tmp/'$om_fhicl'2.fcl' \
    	-c 'sed -r -i "s/.*modulus.*[0-9]+.*/modulus: 100/" /tmp/'$om_fhicl'2.fcl' \
    	-c 'sed -r -i "/end_paths:/s/a3/a1/" /tmp/'$om_fhicl'2.fcl' \
    	-c 'sed -r -i "/shm_key:/s/.*/shm_key: 0x40471453/" /tmp/'$om_fhicl'2.fcl' \
    	-c 'sed -r -i "s/shmem1/shmem2/"  /tmp/'$om_fhicl'2.fcl' \
		-c 'sed -r -i "s/destination_rank: 6/destination_rank: 7/" /tmp/'$om_fhicl'2.fcl' \
        -c 'art -c  /tmp/'$om_fhicl'2.fcl'

	fi
fi
