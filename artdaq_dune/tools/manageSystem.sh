#!/bin/bash

AGGREGATOR_NODE=`hostname`
THIS_NODE=`hostname -s`

# this function expects a number of arguments:
#  1) the DAQ command to be sent
#  2) the run number (dummy for non-start commands)
#  3) where the onmon output file should be saved (if enabled in config)
#  4) path to the configuration
#  5) the logfile name
function launch() {
  echo "Running: DemoControl.rb -s -c $1 --run-number $2 --onmon-file $3 --config-file $4 ${opt_v:+--serialize} ${opt_m:+--online-monitoring $opt_m} 2>&1 | tee -a $5"
  DemoControl.rb -s -c $1 \
    --run-number $2 --onmon-file $3 \
    --config-file $4 \
    ${opt_v:+--serialize} ${opt_m:+--online-monitoring $opt_m} \
    2>&1 | tee -a ${5}
}

scriptName=`basename $0`
usage () {
    echo "
Usage: ${scriptName} [options] <command>
Where command is one of:
  init, start, pause, resume, stop, status, get-legal-commands,
  shutdown, start-system, restart, reinit, exit,
  fast-shutdown, fast-restart, fast-reinit, or fast-exit
General options:
  -h, --help: prints this usage message
  -C: Configuration file to pass to DemoControl (required)
  -v: be more verbose (e.g. dump .fcl (fhicl) config files used during init)
Configuration options (init commands):
  -M <onmon dir>: Where to store onmon output file (if enabled in config)
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
Examples: ${scriptName} init
          ${scriptName} -N 101 start
" >&2
}

# parse the command-line options
originalCommand="$0 $*"
runNumber=""
OPTIND=1
onmonFile="/dev/null"
configName=""
while getopts "hvN:M:C:m:-:" opt; do
    if [ "$opt" = "-" ]; then
        opt=$OPTARG
    fi
    case $opt in
        h|help)  usage; exit 1                    ;;
        C)       configName=${OPTARG}             ;;
        N)       runNumber=${OPTARG}              ;;
        M)       onmonFile=${OPTARG}              ;;
        v)       opt_v=1                          ;;
        m)       test $OPTARG = on && OPTARG=1;opt_m=${opt_m:+$opt_m,}${OPTARG} ;;
        *)       usage; exit 1                    ;;
    esac
done
shift $(($OPTIND - 1))

# fetch the command to run
if [ $# -lt 1 ]; then
    usage
    exit
fi
command=$1
shift

source `which setupDemoEnvironment.sh` ""


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
    usage
    exit
fi

# verify that the expected arguments were provided
if [[ "$configName" == "" ]]; then
    echo ""
    echo "*** You must specify a configuration file"
    usage
    exit
fi
if [[ "$command" == "start" ]] && [[ "$runNumber" == "" ]]; then
    echo ""
    echo "*** A run number needs to be specified."
    usage
    exit
fi

# fill in values for options that weren't specified
if [[ "$runNumber" == "" ]]; then
    runNumber=101
fi

# build the logfile name
TIMESTAMP=`date '+%Y%m%d%H%M%S'`
# Discover the log directory
logroot=`grep -oE "<logDir>(.*?)</logDir>" $configName|sed -r 's/<\/?logDir>//g'`
logFile="${logroot}/masterControl/dsMC-${TIMESTAMP}-${command}.log"
echo "${originalCommand}" > $logFile
echo ">>> ${originalCommand} (Disk writing is ${diskWriting})"

# calculate the shmkey that should be checked
let shmKey=1078394880+${ARTDAQDEMO_PMT_PORT}
shmKeyString=`printf "0x%x" ${shmKey}`

# invoke the requested command
if [[ "$command" == "shutdown" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $onmonFile $configName $logFile
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown"  $runNumber $onmonFile $configName $logFile
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
elif [[ "$command" == "start-system" ]]; then
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "restart" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop"  $runNumber $onmonFile $configName $logFile
   # next send a shutdown command to move the processes to their ground state
    launch "shutdown"  $runNumber $onmonFile $configName $logFile
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    # start the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "reinit" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop"  $runNumber $onmonFile $configName $logFile
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown"  $runNumber $onmonFile $configName $logFile
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    # start the MPI program
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
    # send the init command to re-initialize the system
    sleep 5
    launch "init"  $runNumber $onmonFile $configName $logFile
elif [[ "$command" == "exit" ]]; then
    launch "shutdown"  $runNumber $onmonFile $configName $logFile
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.exit
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"

elif [[ "$command" == "fast-shutdown" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
elif [[ "$command" == "fast-restart" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "fast-reinit" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.startSystem
    sleep 5
    launch "init"  $runNumber $onmonFile $configName $logFile
elif [[ "$command" == "fast-exit" ]]; then
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${ARTDAQDEMO_PMT_PORT}/RPC2 pmt.exit
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"

else
    launch $command  $runNumber $onmonFile $configName $logFile
fi
