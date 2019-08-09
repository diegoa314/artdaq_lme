#!/bin/bash

configFile=""
while getopts "C:c:-:" opt; do
  if [ "$opt" = "-" ]; then
    opt=$OPTARG
  fi
  case $opt in
    C | c )
      configFile=${OPTARG}
      ;;
    config-file)
      configFile=${!OPTIND}
      let OPTIND=$OPTIND+1
      ;;
  esac
done
shift $(($OPTIND - 1))

function makeLogDirs() {
  mkdir -p -m 0777 $1/pmt
  mkdir -p -m 0777 $1/masterControl
  mkdir -p -m 0777 $1/boardreader
  mkdir -p -m 0777 $1/eventbuilder
  mkdir -p -m 0777 $1/aggregator
mkdir -p -m 0777 $1/artdaqart

}

source `which setupDemoEnvironment.sh`

if [ "x$configFile" == "x" ]; then
  
  # create the configuration file for PMT
  tempFile="/tmp/pmtConfig.$$"

  echo "BoardReaderMain `hostname` ${ARTDAQDEMO_BR_PORT[0]}" >> $tempFile
  echo "BoardReaderMain `hostname` ${ARTDAQDEMO_BR_PORT[1]}" >> $tempFile
  echo "EventBuilderMain `hostname` ${ARTDAQDEMO_EB_PORT[0]}" >> $tempFile
  echo "EventBuilderMain `hostname` ${ARTDAQDEMO_EB_PORT[1]}" >> $tempFile
  echo "DataLoggerMain `hostname` ${ARTDAQDEMO_AG_PORT[0]}" >> $tempFile
  echo "DispatcherMain `hostname` ${ARTDAQDEMO_AG_PORT[1]}" >> $tempFile

  # create the logfile directories, if needed
  logroot="${ARTDAQDEMO_LOG_DIR:-/tmp}"
  makeLogDirs $logroot
  
  # start PMT
  pmt.rb -p ${ARTDAQDEMO_PMT_PORT} -d $tempFile --logpath ${logroot} --display ${DISPLAY}
  rm $tempFile
else
  echo "Config File is $configFile"

  # Discover the log directory
  logroot=`grep -oE "<logDir>(.*?)</logDir>" $configFile|sed -r 's/<\/?logDir>//g'`
  makeLogDirs $logroot

  # start PMT
  pmt.rb -p ${ARTDAQDEMO_PMT_PORT} -C $configFile --logpath ${logroot} --display ${DISPLAY}
fi
