#!/bin/sh
 # This file (disk_stats.sh) was created by Ron Rechenmacher <ron@fnal.gov> on
 # Jun 28, 2018. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: disk_stats.sh,v $
 # rev='$Revision: 1.2 $$Date: 2018/06/30 02:57:58 $'

USAGE="\
  usage: `basename $0` <drive.fcl> <directories>...
example:
  rm -fr /data[0123]/disk_stat_test; mkdir /data{0,1,2,3}/disk_stat_test;\\
  `basename $0` \$ARTDAQ_DEMO_DIR/tools/fcl/driver_test1a.fcl /data{0,1,2,3}/disk_stat_test;\\
  gnuplot -e png=0 \`/bin/ls -t periodic_*_stats.out|head -1\`
NOTES:
  This test will start artdaqDriver processes for each directory specified.
  It will then get disk stats and also CPU stats for artdaqDriver and art process running.
  No other artdaqDriver or art process should be running during this test.
"

test $# -lt 2 && { echo "$USAGE"; exit 1; }
set -u

fcl_file=$1; shift
dirs=$*

# find disks for all dirs
disks=
for dd in $dirs;do
   dsk=`df $dd | sed -n '/^.dev/{s|/dev/||;s/ .*//;p;}'`
   test -z "$dsk" && { echo "Error - can not determine disk from $dd"; exit 1; }
   disks="${disks:+$disks,}$dsk"
done
echo "disks=$disks"

dirsav=`pwd`
pids=
for dd in $dirs;do
    cd $dd;
    dsk=`df . | sed -n '/^.dev/{s|/dev/||;s/ .*//;p;}'`
    artdaqDriver -c $fcl_file >$dirsav/artdaqDriver.$dsk.out &
    pids="${pids:+$pids }$!"
    cd $dirsav
done
num_pids=`echo $pids | wc -w`
sleep 1
# see if artdaqDriver processes are (still) up (via kill -0)
start=`date +%s`
while true;do
    ok=1
    for pp in $pids;do
        kill -0 $pp >/dev/null 2>&1 || { ok=0; sleep 1; break; } 
    done
    now=`date +%s`
    test $ok -eq 1 -o `expr \( $now - $start \)` -gt 10 && break;
done
test $ok -eq 0 && { echo "Error - processes took more than 10 seonds to start. Check artdaqDriver output files."; exit 1; }
sleep 1; # allow time for all art processes to start -- perhaps loading (libraries off of nfs
psout=`ps aux`
num_art=`echo "$psout" | grep ' art ' | wc -l`
echo num_art=$num_art

periodic_cmd_stats --comment="$fcl_file" --pid="`pidof artdaqDriver`,`pidof art`"\
 --disk=$disks --sys-iowait
