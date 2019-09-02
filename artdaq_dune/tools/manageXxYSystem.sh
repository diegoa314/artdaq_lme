#!/bin/bash

scriptName=`basename $0`
USAGE="
Usage: ${scriptName} <.cfg file> [options] <command>
Where command is one of:
  init, start, pause, resume, stop, status, get-legal-commands,
  shutdown, start-system, restart, reinit, exit,
  fast-shutdown, fast-restart, fast-reinit, or fast-exit
General options:
  -h, --help: prints this usage message
Configuration options (init commands):
  -m <on|off>: specifies whether to run online monitoring [default=off]
  -D : disables the writing of data to disk
  -c <compression level>: specifies the ADC data compression level
      0 = no compression
      1 = compression, both raw and compressed data kept [default]
      2 = compression, only compressed data kept
  -o <data dir>: specifies the directory for data files [default=${ARTDAQDEMO_DATA_DIR:-/tmp}]
Begin-run options (start command):
  -N <run number>: specifies the run number
Notes:
  The start command expects a run number to be specified (-N).
  The primary commands are the following:
   * init - initializes (configures) the DAQ processes
   * start - starts a run
   * pause - pauses the run
   * resume - resumes the run
   * stop - stops the run
   * status - checks the status of each DAQ process
   * get-legal-commands - fetches the legal commands from each DAQ process
  Additional commands include:
   * shutdown - stops the run (if one is going), resets the DAQ processes
       to their ground state (if needed), and stops the MPI program (DAQ processes)
   * start-system - starts the MPI program (the DAQ processes)
   * restart - this is the same as a shutdown followed by a start-system
   * reinit - this is the same as a shutdown followed by a start-system and an init
   * exit - this resets the DAQ processes to their ground state, stops the MPI
       program, and exits PMT.
  Expert-level commands:
   * fast-shutdown - stops the MPI program (all DAQ processes) no matter what
       state they are in. This could have bad consequences if a run is going!
   * fast-restart - this is the same as a fast-shutdown followed by a start-system
   * fast-reinit - this is the same as a fast-shutdown followed by a start-system
       and an init
   * fast-exit - this stops the MPI program, and exits PMT.
Examples: $scriptName 2x2     -p 32768 init
          $scriptName 2x2.cfg -N 101 start
          $scriptName started -N 101 start
.cfg file format:
<exe> <node> <port>
example .cfg:
BoardReaderMain mu2edaq05-data 5305
BoardReaderMain mu2edaq06-data 5306
EventBuilderMain mu2edaq05-data 5335
EventBuilderMain mu2edaq06-data 5336
"
# set env including ARTDAQDEMO_PMT_PORT, resets dataDir
source `which setupDemoEnvironment.sh` ""
# defaults
originalCommand="$0 $*"
compressionLevel=1
onmonEnable=off
diskWriting=1
dataDir="${ARTDAQDEMO_DATA_DIR:-/tmp}"
runNumber=""
fileSize=0
fsChoiceSpecified=0
fileEventCount=0
fileDuration=0
verbose=0
# parse the command-line options
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`; test -n "$rest" && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`; test -n "$rest" && set -- "$rest"  "$@"'
args=
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        case "$op" in
        h|\?|-help) echo "$USAGE";exit;;
        v*)  eval $op1chr; verbose=1;;
        D*)  eval $op1chr; diskWriting=0;;
        c*)  eval $op1arg; compressionLevel="$1";shift;;
        N*)  eval $op1arg; runNumber="$1";shift;;
        m*)  eval $op1arg; onmonEnable="$1";shift;;
        o*)  eval $op1arg; dataDir="$1";shift;;
        *)   echo "Unknown opt -$op"; echo "$USAGE"; exit 1;;
        esac
    else
        aa=`echo "$1" | sed -e "s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa
test $# -lt 2 && { echo "Invalid number of args $#"; echo "$USAGE"; exit 1; }

# fetch the config and command to run
cf=
test             -f $1.cfg && { cf=$1.cfg; cfgdir=$1.d; }
test -z "$cf" -a -f $1     && { cf=$1      cfgdir=`dirname $1`/`basename $1 .cfg`; }
test -z "$cf" && { echo "cfg file not found"; exit 1; }
command=$2;
shift 2

cfd=`dirname $cf` # config file dirname
cfd=`cd $cfd >/dev/null 2>&1;pwd`
cf="$cfd/`basename $cf`"
test -d "$cfgdir" && cd "$cfgdir"

# verify that the command is one that we expect
if [[ "$command" != "start-system" ]] && \
   [[ "$command" != "init" ]] && \
   [[ "$command" != "start" ]] && \
   [[ "$command" != "pause" ]] && \
   [[ "$command" != "resume" ]] && \
   [[ "$command" != "stop" ]] && \
   [[ "$command" != "status" ]] && \
   [[ "$command" != "get-legal-commands" ]] && \
   [[ "$command" != "shutdown" ]] && \
   [[ "$command" != "fast-shutdown" ]] && \
   [[ "$command" != "restart" ]] && \
   [[ "$command" != "fast-restart" ]] && \
   [[ "$command" != "reinit" ]] && \
   [[ "$command" != "fast-reinit" ]] && \
   [[ "$command" != "exit" ]] && \
   [[ "$command" != "fast-exit" ]]; then
    echo "Invalid command."
    echo "$USAGE"
    exit 1
fi

# verify that the expected arguments were provided
if [[ "$command" == "start" ]] && [[ "$runNumber" == "" ]]; then
    echo ""
    echo "*** A run number needs to be specified."
    usage
    exit 1
fi

# fill in values for options that weren't specified
if [[ "$runNumber" == "" ]]; then
    runNumber=101
fi

# translate the onmon enable flag
if [[ "$onmonEnable" == "on" ]]; then
    onmonEnable=1
else
    onmonEnable=0
fi

# build the logfile name
TIMESTAMP=`date '+%Y%m%d%H%M%S'`
logFile="${ARTDAQDEMO_LOG_DIR:-/tmp}/masterControl/dsMC-${TIMESTAMP}-${command}.log"
echo "${originalCommand}" > $logFile
echo ">>> ${originalCommand} (Disk writing is ${diskWriting})"

# calculate the shmkey that should be checked
let shmKey=1078394880+${ARTDAQDEMO_PMT_PORT}
shmKeyString=`printf "0x%x" ${shmKey}`


# this function expects a number of arguments:
#  1) the DAQ command to be sent
#  2) the run number (dummy for non-start commands)
#  3) the compression level for ADC data [0..2] # UNUSED
#  4) whether to run online monitoring
#  5) the data directory
#  6) the logfile name
#  7) whether to write data to disk [0,1]
#  8) the desired size of each data file
#  9) the desired number of events in each file
# 10) the desired time duration of each file (minutes)
# 11) whether to print out CFG information (verbose)
function launch() {
  enableSerial=""
  if [[ "${11}" == "1" ]]; then
      enableSerial="-e"
  fi

  cfg_ops=`cat $cf | awk '
/BoardReader/{printf "--toy%d %s,%s,%d\n",t+1,$2,$3,b;t=xor(t,1);++b}
/EventBuilder/{printf "--eb %s,%s\n",$2,$3}
'`
  DemoControl.rb ${enableSerial} -s -c $1 \
    $cfg_ops \
    --data-dir ${5} --online-monitoring $4 \
    --write-data ${7} --file-size ${8} \
    --file-event-count ${9} --file-duration ${10} \
    --run-number $2 2>&1 | tee -a ${6}
}

THIS_NODE=`hostname -s`

# invoke the requested command
if [[ "$command" == "shutdown" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem

elif [[ "$command" == "start-system" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem

elif [[ "$command" == "restart" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    # start the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem

elif [[ "$command" == "reinit" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    # start the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
    # send the init command to re-initialize the system
    sleep 5
    launch "init" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose

elif [[ "$command" == "exit" ]]; then
    launch "shutdown" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.exit

elif [[ "$command" == "fast-shutdown" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem

elif [[ "$command" == "fast-restart" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem

elif [[ "$command" == "fast-reinit" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
    sleep 5
    launch "init" $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
elif [[ "$command" == "fast-exit" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.exit

else
    launch $command $runNumber $compressionLevel $onmonEnable $dataDir \
        $logFile $diskWriting $fileSize \
        $fileEventCount $fileDuration $verbose
fi
