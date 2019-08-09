#! /bin/bash
#  This file (artdaq-demo-quickstart.sh) was created by Ron Rechenmacher <ron@fnal.gov> on
#  Jan  7, 2014. "TERMS AND CONDITIONS" governing this file are in the README
#  or COPYING file. If you do not have such a file, one can be obtained by
#  contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
#  $RCSfile: .emacs.gnu,v $
rev='$Revision: 1.20 $$Date: 2010/02/18 13:20:16 $'
#
#  This script is based on the original createArtDaqDemo.sh script created by
#  Kurt and modified by John.

# This script is stored at:
#        https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/repository/revisions/develop/raw/tools/artdaq-demo-quick-start.sh
# and can be retrieved via wget:
#   wget https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/repository/revisions/develop/raw/tools/artdaq-demo-quick-start.sh

#
# This script will:
#      1.  get (if not already gotten) artdaq-demo and all support products
#          Note: this whole demo takes approx. 4 Gigabytes of disk space
#      2a. possibly build the artdaq dependent product
#      2b. build (via cmake),
#  and 3.  start the artdaq-demo system
#

# program (default) parameters
root=
tag=
productsdir=

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options] [demo_root]
examples: `basename $0` .
          `basename $0` --run-demo
          `basename $0` --HEAD --debug
If the \"demo_root\" optional parameter is not supplied, the user will be
prompted for this location.
--run-demo    runs the demo
--debug       perform a debug build
--viewer      install and run the artdaq Message Viewer
-f            force download
--skip-check  skip the free diskspace check
--HEAD        all git repo'd packages checked out from HEAD of develop branches
--tag         check out a specific tag of artdaq-demo
-e, -s        Use speific qualifiers when building ARTDAQ (both must be specified
              to have any effect).
-v            Be more verbose
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_v=0
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
            \?*|h*)     eval $op1chr; do_help=1;;
            v*)         eval $op1chr; opt_v=`expr $opt_v + 1`;;
            x*)         eval $op1chr; set -x;;
            f*)         eval $op1chr; opt_force=1;;
            t*|-tag)    eval $reqarg; tag=$1;    shift;;
            s*)         eval $op1arg; squalifier=$1; shift;;
            e*)         eval $op1arg; equalifier=$1; shift;;
            -products-dir)    eval $reqarg; productsdir=$1;    shift;;
            -skip-check)opt_skip_check=1;;
            -run-demo)  opt_run_demo=--run-demo;;
			-debug)     opt_debug=--debug;;
			-viewer) opt_viewer=--viewer;;
			-HEAD)  opt_HEAD=--HEAD;;
            *)          echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa
set -u   # complain about uninitialed shell variables - helps development

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit
test $# -eq 1 && root=$1

if [ $opt_v -gt 0 ]; then
	set -x
fi

#check that $0 is in a git repo
tools_path=`dirname $0`
tools_path=`cd "$tools_path" >/dev/null;pwd`
tools_dir=`basename $tools_path`
git_working_path=`dirname $tools_path`
cd "$git_working_path" >/dev/null
git_working_path=$PWD

if [ -z "$root" ];then
    root=`dirname $git_working_path`
    echo the root is $root
fi
test -d "$root" || mkdir -p "$root"

# JCF, 1/16/15
# Save all output from this script (stdout + stderr) in a file with a
# name that looks like "quick-start.sh_Fri_Jan_16_13:58:27.script" as
# well as all stderr in a file with a name that looks like
# "quick-start.sh_Fri_Jan_16_13:58:27_stderr.script"
alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )
stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )
mkdir -p "$root/log"
exec  > >(tee "$root/log/$alloutput_file")
exec 2> >(tee "$root/log/$stderr_file")

git_status=`git status 2>/dev/null`
git_sts=$?
if [ $git_sts -ne 0 -o $tools_dir != tools ];then
    echo problem with git or quick-start.sh script is not from/in a git repository
    exit 1
fi

branch=`git branch | sed -ne '/^\*/{s/^\* *//;p;q}'`
echo the current branch is $branch
# The initial clone will have branch = develop.
# In this case, IF opt_HEAD is not set (or, actually, has zero length),
# THEN checkout tag (latest tag if not specified).
if [ "$branch" = develop -a -z "${opt_HEAD-}" ];then
    test -z "$tag" && tag=`git tag -l 'v[0-9]*' | tail -n1`
    git status | grep -q 'working directory clean' || git stash
    echo "checking out tag $tag"
    git checkout $tag
elif [ -n "${opt_HEAD-}" -a "$branch" != develop ];then # opt_HEAD is set (nonzero length)
    echo "checking out develop"
    git checkout develop
else
    echo "no checkout -- branch = $branch"
fi

# JCF, 8/28/14

# Now that we've checked out the artdaq-demo version we want, make
# sure we know what qualifier is meant to be passed to the
# downloadDeps.sh and installArtDaqDemo.sh scripts below

if [[ -n "${opt_debug:-}" ]] ; then
    build_type="debug"
else
    build_type="prof"
fi

# Get the Artdaq-demo default qualifier
add_defaultqual=`grep ^defaultqual $git_working_path/ups/product_deps | awk '{print $2}'`
# Use that to find the corresponding ARTDAQ qualifier
ad_qual=`grep ^${add_defaultqual}:${build_type} $git_working_path/ups/product_deps | awk '{print $2}'`
# pullProducts expects a qualifier like "s6-e6", get that out of the full ARTDAQ qualifier
defaultqual=`echo $ad_qual|grep -oE "s[0-9]+"`-`echo $ad_qual|grep -oE "e[0-9]+"`
defaultqualWithS=$defaultqual

# JCF, 5/26/15
# More fun - we now want to strip away the "sX" part of the qualifier...
defaultqual=$(echo $defaultqual | sed -r 's/.*(e[0-9]).*/\1/')

# ELF, 11/20/15
# Even more fun - if the user specified a qualifier set, throw this all away...
if [ -n "${equalifier-}" ] && [ -n "${squalifier-}" ]; then
    defaultqual="e${equalifier}"
    defaultqualWithS="s${squalifier}-e${equalifier}"
else
    equalifier=`echo $defaultqualWithS|cut -d'-' -s -f2`
    squalifier=`echo $defaultqualWithS|cut -d'-' -s -f1`
fi

vecho() { test $opt_v -gt 0 && echo "$@"; }
starttime=`date`

cd $root

free_disk_G=`df -B1G . | awk '/[0-9]%/{print$(NF-2)}'`
if [ -z "${opt_skip_check-}" -a "$free_disk_G" -lt 15 ];then
    echo "Error: insufficient free disk space ($free_disk_G). Min. require = 15"
    echo "Use the --skip-check option to skip free space check."
    exit 1
fi

#if [ ! -x $git_working_path/tools/downloadDeps.sh ];then
#    echo error: missing tools/downloadDeps.sh
#    exit 1
#fi
if [ ! -x $git_working_path/tools/installArtDaqDemo.sh ];then
    echo error: missing tools/installArtDaqDemo.sh
    exit 1
fi

# JCF, 1/15/15

# Three scenarios:

# 1) The products-dir argument was not supplied, and either (or both)
# of ./products and ./download do not exist, in which case downloading
# takes place

# 2) The products-dir argument was not supplied, both ./products and
# ./download exist, but since $opt_force is "true" the download
# proceeds regardless

# 3) The products-dir argument was supplied, in which case the
# $productsdir directory is expected to contain all needed packages,
# and no downloading takes place

if [[ ! -n ${productsdir:-} && ( ! -d products || ! -d download || -n "${opt_force-}" ) ]] ; then
    # ELF 5/18/2016: We'll always want these directories to be there, even if they're forcing...
    test -d products || mkdir products
    test -d download || mkdir download
    if [[ ! -n "${opt_force-}" ]]; then
        echo "Are you sure you want to download and install the artdaq demo dependent products in `pwd`? [y/n]"
        read response
        if [[ "$response" != "y" ]]; then
            echo "Aborting..."
            exit
        fi
    else
        echo "Will force download despite existing directories"
    fi

    # ELF 8/17/2015

    # Latest artdaq (v1_12_12) once again has manifests in all the right places. Switching back to
    # bundle-based distribution.

    cd download
    wget http://scisoft.fnal.gov/scisoft/bundles/tools/pullProducts
    chmod +x pullProducts
    version=`grep "^artdaq " $git_working_path/ups/product_deps | awk '{print $2}'`
    
    echo "Cloning cetpkgsupport to determine current OS"
    git clone http://cdcvs.fnal.gov/projects/cetpkgsupport
    os=`./cetpkgsupport/bin/get-directory-name os`

	if [[ "$os" == "u14" ]]; then
		echo "-H Linux64bit+3.19-2.19" >../products/ups_OVERRIDE.`hostname`
    fi
	if [[ `echo $os|grep -c Linux` -gt 0 ]] || [[ `echo $os|grep -c rolling` -gt 0 ]]; then
		osnumMajor=`uname -r|cut -f1 -d.`
		osnumMinor=`uname -r|cut -f2 -d.`
		if [ $osnumMajor -ge 3 ]; then
			if [ $osnumMinor -ge 19 ] || [ $osnumMajor -gt 3 ]; then
				echo "-H Linux64bit+3.19-2.19" >../products/ups_OVERRIDE.`hostname`
				os="u14"
			elif [ $osnumMinor -ge 10 ];then
				echo "-H Linux64bit+3.10-2.12" >../products/ups_OVERRIDE.`hostname`
				os="slf7"
			else
				echo "-H Linux64bit+2.6-2.12" >../products/ups_OVERRIDE.`hostname`
				os="slf6"
			fi
		elif [ $osnumMajor -ge 2 ] && [ $osnumMinor -ge 6 ]; then
			echo "-H Linux64bit+2.6-2.12" >../products/ups_OVERRIDE.`hostname`
			os="slf6"
		else
			echo "Incompatible OS detected! Please upgrade to a newer (2.6+) kernel!"
		fi
    fi

    echo "Running ./pullProducts ../products ${os} artdaq-${version} $defaultqualWithS $build_type"
    ./pullProducts ../products ${os} artdaq-${version} $defaultqualWithS $build_type
    
    if [ $? -ne 0 ]; then
		echo "Error in pullProducts. Please go to http://scisoft.fnal.gov/scisoft/bundles/artdaq/${version}/manifest and make sure that a manifest for the specified qualifiers ($defaultqualWithS) exists."
		exit 1
    fi
    cd ..

elif [[ -n ${productsdir:-} ]] ; then 

    if [[ ! -d $productsdir ]] ; then
		echo 'Unable to find products directory "'$productsdir'", ' \
			"aborting..."
		exit 1
    else
		echo "Will assume all needed products can be found in " \
			"$productsdir; no downloading will be performed"
    fi
fi


$git_working_path/tools/installArtDaqDemo.sh ${productsdir:-products} $git_working_path ${opt_debug-} ${opt_HEAD-} --quals ${defaultqualWithS}

installStatus=$?

source products/setup
upsflavor=`ups flavor`
qt_installed=`ups list -aK+ qt v5_6_1a -q$equalifier -f$upsflavor|grep -c "qt"`
amfver=`curl http://scisoft.fnal.gov/scisoft/packages/artdaq_mfextensions/ 2>/dev/null|grep artdaq_mfextensions|grep "id=\"v"|tail -1|sed 's/.* id="\(v.*\)".*/\1/'`
mfe_installed=`ups list -aK+ artdaq_mfextensions $amfver -q$equalifier:$squalifier:$build_type -f$upsflavor|grep -c "artdaq_mfextensions"`
if [ $installStatus -eq 0 ] &&  [ "x${opt_viewer-}" != "x" ] && [ $qt_installed -eq 0 -o $mfe_installed -eq 0 ]; then
    echo "Installing artdaq_mfextensions"
    # JCF, May-13-2016

    # A simple brute-force download of the packages which will be needed for this example

    cd download
    if ! [ -d "./cetpkgsupport" ]; then
		echo "Cloning cetpkgsupport to determine current OS"
		git clone http://cdcvs.fnal.gov/projects/cetpkgsupport
	fi
	if [ -n ${os-} ];then
		os=`./cetpkgsupport/bin/get-directory-name os`
	fi

	packagelist=""
	amfdotver=`echo $amfver|sed 's/_/\./g'|sed 's/v//'`
	packagelist="$packagelist artdaq_mfextensions/$amfver/artdaq_mfextensions-$amfdotver-${os}-x86_64-${equalifier}-${squalifier}-$build_type.tar.bz2"
	packagelist="$packagelist qt/v5_6_1a/qt-5.6.1a-${os}-x86_64-${equalifier}.tar.bz2"

	for packagehtml in $packagelist ; do
		echo "Downloading http://scisoft.fnal.gov/scisoft/packages/${packagehtml}..."
		wget http://scisoft.fnal.gov/scisoft/packages/$packagehtml > /dev/null 2>&1
		
		packagename=$( echo $packagehtml | awk 'BEGIN { FS="/" } { print $NF }' )

		if [[ ! -e $packagename ]]; then
			echo "Unable to download $packagename"
			exit 1
		fi

		downloaddir=$PWD
		cd ../products
		echo "De-archiving $packagename ("$( stat -c %s $downloaddir/$packagename)" bytes)..."
		tar -xjf $downloaddir/$packagename

		cd $downloaddir
	done
	cd ..
fi

if [ "x${opt_viewer-}" != "x" ]; then
	echo "setup artdaq_mfextensions $amfver -q$equalifier:$squalifier:$build_type" >>./setupARTDAQDEMO
fi

if [ $installStatus -eq 0 ] && [ "x${opt_run_demo-}" != "x" ]; then
	echo doing the demo

	. $git_working_path/tools/run_demo.sh $root $git_working_path/tools

elif [ $installStatus -eq 0 ]; then
	echo "artdaq-demo has been installed correctly. Please see: "
	echo "https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/wiki/Running_a_sample_artdaq-demo_system"
	echo "for instructions on how to run, or re-run this script with the --run-demo option"
	echo
else
	echo "BUILD ERROR!!! SOMETHING IS VERY WRONG!!!"
	echo
fi

endtime=`date`

echo "Build start time: $starttime"
echo "Build end time:   $endtime"
