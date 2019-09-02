#!/bin/bash
# Determines the appropriate version of artdaq_mfextensions to install, and also fetches its dependencies.

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
pcount=`ups list -aK+ artdaq_mfextensions -q $equal:$squal:$btype|grep -c artdaq_mfextensions`
if [ $pcount -gt 0 ]; then
  echo "artdaq_mfextensions already installed. Checking dependencies..."
else

source ./setups
setup cetpkgsupport
os=`get-directory-name os`


mfextensions_ver=`cat $ARTDAQ_DIR/ups/artdaq.table|grep -m 1 artdaq_mfextensions|sed 's/.*artdaq_mfextensions.*\(v[0-9_]*[a-z]\?\).*/\1/'`
mfextensions_dotver=`echo $mfextensions_ver|sed 's/v//'|sed 's/_/./g'`

curl http://scisoft.fnal.gov/scisoft/packages/artdaq_mfextensions/$mfextensions_ver/artdaq_mfextensions-${mfextensions_dotver}-${os}-x86_64-$equal-$squal-$btype.tar.bz2|tar -jx --

fi

setup artdaq_mfextensions $mfextensions_ver -q $equal:$squal:$btype

if [ $? -ne 0 ]; then

qt_ver=`cat artdaq_mfextensions/${mfextensions_ver}/ups/artdaq_mfextensions.table|grep -m 1 qt|sed 's/.*qt.*\(v[0-9_]*[a-z]\?\).*/\1/'`
qt_dotver=`echo $qt_ver|sed 's/v//'|sed 's/_/./g'`

curl http://scisoft.fnal.gov/scisoft/packages/qt/$qt_ver/qt-${qt_dotver}-${os}-x86_64-${equal}.tar.bz2|tar -jx --

fi

setup artdaq_mfextensions $mfextensions_ver -q $equal:$squal:$btype && echo "artdaq_mfextensions version ${mfextensions_ver} successfully installed!"
