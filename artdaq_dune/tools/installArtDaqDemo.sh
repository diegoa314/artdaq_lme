#!/bin/bash
echo Invoked: $0 "$@"
env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
  usage: `basename $0` [options] <demo_products_dir/> <artdaq-demo/>
example: `basename $0` products artdaq-demo --run-demo
<demo_products>    where products were installed (products/)
<artdaq-demo_root> directory where artdaq-demo was cloned into.
--HEAD        all git repo'd packages checked out from HEAD of develop branches
--debug      perform a debug build
-c           \"clean\" build dirs -- may be need during development
--quals      pullProducts-style qualifiers to use for build
Currently this script will clone (if not already cloned) artdaq
along side of the artdaq-demo dir.
Also it will create, if not already created, build directories
for artdaq and artdaq-demo.
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
        \?*|h*)    eval $op1chr; do_help=1;;
        v*)        eval $op1chr; opt_v=`expr $opt_v + 1`;;
        x*)        eval $op1chr; set -x;;
	    -HEAD) opt_HEAD=--HEAD;;
        -debug)    opt_debug=--debug;;
        -quals)    eval $op1arg; qualifiers=$1; shift;;
        c*)        eval $op1chr; opt_clean=1;;
        *)         echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa

test -n "${do_help-}" -o $# -ne 2 && echo "$USAGE" && exit 2

test -d $1 || { echo "products directory ($1) not found"; exit 1; }
products_dir=`cd "$1" >/dev/null;pwd`
artdaq_demo_dir=`cd "$2" >/dev/null;pwd`
demo_dir=`dirname "$artdaq_demo_dir"`

equalifier=`echo $qualifiers|cut -d'-' -s -f2`
squalifier=`echo $qualifiers|cut -d'-' -s -f1`
echo "Building ARTDAQ-DEMO with qualifiers: $equalifier:$squalifier:eth"

export CETPKG_INSTALL=$products_dir
export CETPKG_J=16

test -d "$demo_dir/build_artdaq-demo" || mkdir "$demo_dir/build_artdaq-demo" 

if [[ -n "${opt_debug:-}" ]];then
    build_arg="d"
else
    build_arg="p"
fi

cd $demo_dir >/dev/null  # potential git clones under here

REPO_PREFIX=http://cdcvs.fnal.gov/projects
#REPO_PREFIX=ssh://p-artdaq@cdcvs.fnal.gov/cvs/projects # p-artdaq can be used to access artdaq-demo

function install_package {
    local packagename=$1
    local commit_tag=$2

    # Get rid of the first two positional arguments now that they're stored in named variables
    shift;
    shift;

    test -d "$demo_dir/build_$packagename" || mkdir "$demo_dir/build_$packagename"    

    test -d ${packagename} || git clone $REPO_PREFIX/$packagename
    cd $packagename
    git fetch origin
    git checkout $commit_tag
    cd ../build_$packagename

    echo IN $PWD: about to . ../$packagename/ups/setup_for_development
    . ../$packagename/ups/setup_for_development -${build_arg} $equalifier $squalifier $@
    echo FINISHED ../$packagename/ups/setup_for_development
    buildtool ${opt_clean+-c} -i && res=0 || res=1
    cd ..
    return $res
}

. $products_dir/setup

# ELF 11/16/15
# Removing artdaq-core from the develop products list, as it frequently changes.
# I'm assuming that anyone who wants to do development in artdaq-core can figure
# out how to get it anyway.

# Versions updated 09/30/16
install_package artdaq-core v1_05_09 || exit 1

if [ -n "${opt_HEAD-}" ];then
install_package artdaq-core-demo develop || exit 1
else
install_package artdaq-core-demo v1_05_04 || exit 1
fi

if [ -n "${opt_HEAD-}" ];then
    install_package artdaq-utilities develop || exit 1
else
    install_package artdaq-utilities v1_00_10 || exit 1
fi

# JCF, Sep-29-2016

# artdaq commit a7380e333f804c54d44faddf2e4b44d987d69c3c includes
# changes I made to simplify the Transfer plugin interface after the
# v1_13_02 release

if [ -n "${opt_HEAD-}" ];then
install_package artdaq develop eth || exit 1
else
install_package artdaq v1_13_03 eth || exit 1
fi

setup_qualifier="$equalifier $squalifier eth"

if [ ! -e ./setupARTDAQDEMO -o "${opt_clean-}" == 1 ]; then
    cat >setupARTDAQDEMO <<-EOF
	echo # This script is intended to be sourced.

	sh -c "[ \`ps \$\$ | grep bash | wc -l\` -gt 0 ] || { echo 'Please switch to the bash shell before running the artdaq-demo.'; exit; }" || exit

	source $products_dir/setup

	export CETPKG_INSTALL=$products_dir
	export CETPKG_J=16
	#export ARTDAQDEMO_BASE_PORT=52200
	export DAQ_INDATA_PATH=$artdaq_demo_dir/test/Generators:$artdaq_demo_dir/inputData

	export ARTDAQDEMO_BUILD="$demo_dir/build_artdaq-demo"
	export ARTDAQDEMO_REPO="$artdaq_demo_dir"
	export FHICL_FILE_PATH=.:\$ARTDAQDEMO_REPO/tools/snippets:\$ARTDAQDEMO_REPO/tools/fcl:\$FHICL_FILE_PATH

	echo changing directory to \$ARTDAQDEMO_BUILD
	cd \$ARTDAQDEMO_BUILD  # note: next line adjusts PATH based one cwd
	. \$ARTDAQDEMO_REPO/ups/setup_for_development -${build_arg} $setup_qualifier

	alias toy1toy2EventDump="art -c $artdaq_demo_dir/artdaq-demo/ArtModules/fcl/toy1toy2Dump.fcl"
	alias rawEventDump="art -c $artdaq_demo_dir/artdaq-demo/ArtModules/fcl/rawEventDump.fcl"
	alias compressedEventDump="art -c $artdaq_demo_dir/artdaq-demo/ArtModules/fcl/compressedEventDump.fcl"
	alias compressedEventComparison="art -c $artdaq_demo_dir/artdaq-demo/ArtModules/fcl/compressedEventComparison.fcl"
	EOF
    #
fi


echo "Building artdaq-demo..."
cd $ARTDAQDEMO_BUILD
. $demo_dir/setupARTDAQDEMO
buildtool ${opt_clean+-c} && exit 0 || exit 1

