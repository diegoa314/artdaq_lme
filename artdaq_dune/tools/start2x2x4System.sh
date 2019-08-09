#!/bin/bash

source `which setupDemoEnvironment.sh`

# create the configuration file for PMT
tempFile="/tmp/pmtConfig.$$"

echo "BoardReaderMain `hostname` ${ARTDAQDEMO_BR_PORT[0]}" >> $tempFile
echo "BoardReaderMain `hostname` ${ARTDAQDEMO_BR_PORT[1]}" >> $tempFile
echo "EventBuilderMain `hostname` ${ARTDAQDEMO_EB_PORT[0]}" >> $tempFile
echo "EventBuilderMain `hostname` ${ARTDAQDEMO_EB_PORT[1]}" >> $tempFile
echo "DataLoggerMain `hostname` ${ARTDAQDEMO_AG_PORT[0]}" >> $tempFile
echo "DataLoggerMain `hostname` ${ARTDAQDEMO_AG_PORT[1]}" >> $tempFile
echo "DataLoggerMain `hostname` ${ARTDAQDEMO_AG_PORT[2]}" >> $tempFile
echo "DispatcherMain `hostname` ${ARTDAQDEMO_AG_PORT[3]}" >> $tempFile

# create the logfile directories, if needed
logroot="${ARTDAQDEMO_LOG_DIR:-/tmp}"
mkdir -p -m 0777 ${logroot}/pmt
mkdir -p -m 0777 ${logroot}/masterControl
mkdir -p -m 0777 ${logroot}/boardreader
mkdir -p -m 0777 ${logroot}/eventbuilder
mkdir -p -m 0777 ${logroot}/aggregator
mkdir -p -m 0777 ${logroot}/artdaqart

if [[ "x${ARTDAQ_MFEXTENSIONS_DIR-}" != "x" ]] && [[ "x${DISPLAY-}" != "x" ]]; then
    configPath=$ARTDAQ_MFEXTENSIONS_DIR/config/msgviewer.fcl
    if [ -n "${ARTDAQ_MFEXTENSIONS_FQ_DIR}" ]; then configPath=${ARTDAQ_MFEXTENSIONS_FQ_DIR}/bin/msgviewer.fcl; fi
    msgviewer -c $configPath 2>&1 >${logroot}/msgviewer.log &
    echo "udp: { type: \"UDP\" threshold: \"INFO\" host: \"${HOSTNAME}\" port: 30000 }" >${logroot}/MessageFacility.fcl
    export ARTDAQ_LOG_FHICL=${logroot}/MessageFacility.fcl
fi

# start PMT
pmt.rb -p ${ARTDAQDEMO_PMT_PORT} -d $tempFile --logpath ${logroot} --display ${DISPLAY}
rm $tempFile
