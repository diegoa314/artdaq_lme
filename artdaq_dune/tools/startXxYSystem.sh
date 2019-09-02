#!/bin/bash

USAGE="\
   usage: `basename $0` [opts] <.cfg file>
examples:
  `basename $0` 2x2
  `basename $0` 2x2.cfg
options:
  -h,-? --help
  -o<logroot>
.cfg file format:
<exe> <node> <port>
example .cfg:
BoardReaderMain mu2edaq05-data 5305
BoardReaderMain mu2edaq06-data 5306
EventBuilderMain mu2edaq05-data 5335
EventBuilderMain mu2edaq06-data 5336
"
source `which setupDemoEnvironment.sh`
logroot="${ARTDAQDEMO_LOG_DIR:-/tmp}"
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`; test -n "$rest" && set -- "$rest"  "$@"'
args=
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        case "$op" in
        h|\?|-help) echo "$USAGE";exit;;
        o*)  eval $op1arg; logroot="$1";shift;;
        *)   echo "unknown opt -$op";exit 1;;
        esac
    else
        aa=`echo "$1" | sed -e "s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa
test $# -ne 1 && { echo "Invalid number of args $#"; echo "$USAGE"; exit 1; }
cf=
test             -f $1.cfg && { cf=$1.cfg; cfgdir=$1.d; }
test -z "$cf" -a -f $1     && { cf=$1      cfgdir=`dirname $1`/`basename $1 .cfg`.d; }
test -z "$cf" && { echo "cfg file not found"; exit 1; }
reldir=`dirname $cf`
absdir=`cd $reldir >/dev/null;pwd`
nf=$absdir/`basename $cf .cfg`.node
test -f $nf || { echo Corresponding node file $nf not found; exit 1; }
export NODE_LIST=$nf


/bin/cp -f $cf started.cfg
echo cfgdir=$cfgdir
test -d $cfgdir && { test -L started.d && rm -f started.d; ln -b -s $cfgdir started.d; }

# create the logfile directories, if needed
mkdir -p -m 0777 ${logroot}/pmt
mkdir -p -m 0777 ${logroot}/masterControl
mkdir -p -m 0777 ${logroot}/boardreader
mkdir -p -m 0777 ${logroot}/eventbuilder
mkdir -p -m 0777 ${logroot}/artdaqart

# start PMT
pmt.rb -p ${ARTDAQDEMO_PMT_PORT} -d $cf --logpath ${logroot} --display ${DISPLAY}
