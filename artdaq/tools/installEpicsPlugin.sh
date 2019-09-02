#!/bin/bash
# Determines the appropriate version of artdaq_mfextensions to install, and also fetches its dependencies.

echo "This script will not yet work, as the artdaq_epics_plugin product is not on SciSoft."
echo "If you are interested in the artdaq_epics_plugin, please contact the artdaq developers for a copy"
echo "or 'git clone http://cdcvs.fnal.gov/projects/artdaq-utilities-epics-plugin artdaq-epics-plugin'"
exit 1

function setup() {
 . `$UPS_DIR/bin/ups setup "$@"`
}

# Check the environment
if [ -e "./setups" ] && [ -d "ups" ]; then
	echo "UPS area detected!"
	if [ -z "${ARTDAQ_FQ_DIR-}" ]; then
	echo "Please have the desired version of ARTDAQ set up before running this script!"
		exit 1
	fi
else
	echo "This script must be run from a UPS product directory!"
	exit 1
fi

equal=`echo $ARTDAQ_FQ_DIR|sed 's/.*\(e[0-9]\+\).*/\1/'`
squal=`echo $ARTDAQ_FQ_DIR|sed 's/.*\(s[0-9]\+\).*/\1/'`
btype=`echo $ARTDAQ_FQ_DIR|sed 's/.*\(debug\|prof\).*/\1/'`
pcount=`ups list -aK+ artdaq_epics_plugin -q $equal:$squal:$btype|grep -c artdaq_epics_plugin`
if [ $pcount -gt 0 ]; then
  echo "artdaq_epics_plugin already installed. Checking dependencies..."
else

source ./setups
setup cetpkgsupport
os=`get-directory-name os`


epics_plugin_ver=`cat $ARTDAQ_DIR/ups/artdaq.table|grep -m 1 artdaq_epics_plugin|sed 's/.*artdaq_epics_plugin.*\(v[0-9_]*[a-z]\?\).*/\1/'`
epics_plugin_dotver=`echo $epics_plugin_ver|sed 's/v//'|sed 's/_/./g'`

curl http://scisoft.fnal.gov/scisoft/packages/artdaq_epics_plugin/$epics_plugin_ver/artdaq_epics_plugin-${epics_plugin_dotver}-${os}-x86_64-$equal-$squal-$btype.tar.bz2|tar -jx --

fi

setup artdaq_epics_plugin $epics_plugin_ver -q $equal:$squal:$btype

if [ $? -ne 0 ]; then

epics_ver=`cat artdaq_epics_plugin/${epics_plugin_ver}/ups/artdaq_epics_plugin.table|grep -m 1 epics|sed 's/.*epics.*\(v[0-9_]*[a-z]\?\).*/\1/'`
epics_dotver=`echo $epics_ver|sed 's/v//'|sed 's/_/./g'`

curl http://scisoft.fnal.gov/scisoft/packages/epics/$epics_ver/epics-${epics_dotver}-${os}-x86_64-${equal}.tar.bz2|tar -jx --

fi

setup artdaq_epics_plugin $epics_plugin_ver -q $equal:$squal:$btype && echo "artdaq_epics_plugin version ${epics_plugin_ver} successfully installed!"
