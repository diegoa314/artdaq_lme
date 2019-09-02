#!/bin/bash
# Determines the appropriate version of artdaq_ganglia_plugin to install, and also fetches its dependencies.

gqual=gggg
if [ ! -z "$1" ]; then
gqual=$1
fi

if [[ "$gqual" == "gggg" ]]; then
  echo "To specify a version of Ganglia other than v3_7_1, pass the g-qualifier as an option"
  echo "    e.g. `basename $0` g360"
  gqual=g371
fi

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
pcount=`ups list -aK+ artdaq_ganglia_plugin -q$gqual:$equal:$squal:$btype|grep -c artdaq_ganglia_plugin`
if [ $pcount -gt 0 ]; then
  echo "artdaq_ganglia_plugin already installed. Checking dependencies..."
else

source ./setups
setup cetpkgsupport
os=`get-directory-name os`

ganglia_plugin_ver=`cat $ARTDAQ_DIR/ups/artdaq.table|grep -m 1 artdaq_ganglia_plugin|sed 's/.*artdaq_ganglia_plugin.*\(v[0-9_]*[a-z]\?\).*/\1/'`
ganglia_plugin_dotver=`echo $ganglia_plugin_ver|sed 's/v//'|sed 's/_/./g'`

curl http://scisoft.fnal.gov/scisoft/packages/artdaq_ganglia_plugin/$ganglia_plugin_ver/artdaq_ganglia_plugin-${ganglia_plugin_dotver}-${os}-x86_64-$equal-$gqual-$squal-$btype.tar.bz2|tar -jx --

fi

setup artdaq_ganglia_plugin $ganglia_plugin_ver -q $equal:$squal:$btype:$gqual

if [ $? -ne 0 ]; then

ganglia_ver=v3_7_1
ganglia_dotver=3.7.1
case $gqual in
  g360)
	ganglia_ver=v3_6_0
	ganglia_dotver=3.6.0
	;;
  g317)
	ganglia_ver=v3_1_7
	ganglia_dotver=3.1.7
	;;
esac

curl http://scisoft.fnal.gov/scisoft/packages/ganglia/$ganglia_ver/ganglia-${ganglia_dotver}-${os}-x86_64.tar.bz2|tar -jx --

fi

setup artdaq_ganglia_plugin $ganglia_plugin_ver -q $equal:$squal:$btype:$gqual && echo "artdaq_ganglia_plugin version ${ganglia_plugin_ver} successfully installed!"
