#! /bin/bash
# quick-mrb-start.sh - Eric Flumerfelt, May 20, 2016
# Downloads, installs, and runs the artdaq_demo as an MRB-controlled repository

git_status=`git status 2>/dev/null`
git_sts=$?
if [ $git_sts -eq 0 ];then
	echo "This script is designed to be run in a fresh install directory!"
	exit 1
fi


starttime=`date`
Base=$PWD
test -d products || mkdir products
test -d download || mkdir download
test -d log || mkdir log

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options] [demo_root]
examples: `basename $0` .
		  `basename $0` --run-demo
		  `basename $0` --debug
		  `basename $0` --tag v2_08_04
If the \"demo_root\" optional parameter is not supplied, the user will be
prompted for this location.
--run-demo    runs the demo
--debug       perform a debug build
--develop     Install the develop version of the software (may be unstable!)
--viewer      install and run the artdaq Message Viewer
--mfext       Use artdaq_mfextensions Destinations by default
--tag         Install a specific tag of artdaq_demo
--logdir      Set <dir> as the destination for log files
--datadir     Set <dir> as the destination for data files
-e, -s, -c    Use specific qualifiers when building ARTDAQ
-v            Be more verbose
-x            set -x this script
-w            Check out repositories read/write
--no-extra-products  Skip the automatic use of central product areas, such as CVMFS
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
datadir="${ARTDAQDEMO_DATA_DIR:-$Base/daqdata}"
logdir="${ARTDAQDEMO_LOG_DIR:-$Base/daqlogs}"
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_v=0; opt_w=0; opt_develop=0; opt_skip_extra_products=0;
while [ -n "${1-}" ];do
	if expr "x${1-}" : 'x-' >/dev/null;then
		op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
		leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
		test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
		case "$op" in
			\?*|h*)     eval $op1chr; do_help=1;;
			v*)         eval $op1chr; opt_v=`expr $opt_v + 1`;;
			x*)         eval $op1chr; set -x;;
			s*)         eval $op1arg; squalifier=$1; shift;;
			e*)         eval $op1arg; equalifier=$1; shift;;
            c*)         eval $op1arg; cqualifier=$1; shift;;
			w*)         eval $op1chr; opt_w=`expr $opt_w + 1`;;
			-run-demo)  opt_run_demo=--run-demo;;
			-debug)     opt_debug=--debug;;
			-develop) opt_develop=1;;
			-tag)       eval $reqarg; tag=$1; shift;;
			-viewer)    opt_viewer=--viewer;;
			-logdir)    eval $op1arg; logdir=$1; shift;;
			-datadir)   eval $op1arg; datadir=$1; shift;;
			-no-extra-products)  opt_skip_extra_products=1;;
			-mfext)     opt_mfext=1;;
			*)          echo "Unknown option -$op"; do_help=1;;
		esac
	else
		aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
	fi
done
eval "set -- $args \"\$@\""; unset args aa
set -u   # complain about uninitialed shell variables - helps development

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit

if [[ -n "${tag:-}" ]] && [[ $opt_develop -eq 1 ]]; then 
	echo "The \"--tag\" and \"--develop\" options are incompatible - please specify only one."
	exit
fi


# JCF, 1/16/15
# Save all output from this script (stdout + stderr) in a file with a
# name that looks like "quick-start.sh_Fri_Jan_16_13:58:27.script" as
# well as all stderr in a file with a name that looks like
# "quick-start.sh_Fri_Jan_16_13:58:27_stderr.script"
alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )
stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )
exec  > >(tee "$Base/log/$alloutput_file")
exec 2> >(tee "$Base/log/$stderr_file")

function detectAndPull() {
	local startDir=$PWD
	cd $Base/download
	local packageName=$1
	local packageOs=$2
	if [[ "$packageOs" != "noarch" ]]; then
		local packageOsArch="$2-x86_64"
		packageOs=`echo $packageOsArch|sed 's/-x86_64-x86_64/-x86_64/g'`
	fi

	if [ $# -gt 2 ];then
		local qualifiers=$3
		if [[ "$qualifiers" == "nq" ]]; then
			qualifiers=
		fi
	fi
	if [ $# -gt 3 ];then
		local packageVersion=$4
	else
		local packageVersion=`curl http://scisoft.fnal.gov/scisoft/packages/${packageName}/ 2>/dev/null|grep ${packageName}|grep "id=\"v"|tail -1|sed 's/.* id="\(v.*\)".*/\1/'`
	fi
	local packageDotVersion=`echo $packageVersion|sed 's/_/\./g'|sed 's/v//'`

	if [[ "$packageOs" != "noarch" ]]; then
		local upsflavor=`ups flavor`
		local packageQualifiers="-`echo $qualifiers|sed 's/:/-/g'`"
		local packageUPSString="-f $upsflavor -q$qualifiers"
	fi
	local packageInstalled=`ups list -aK+ $packageName $packageVersion ${packageUPSString-}|grep -c "$packageName"`
	if [ $packageInstalled -eq 0 ]; then
		local packagePath="$packageName/$packageVersion/$packageName-$packageDotVersion-${packageOs}${packageQualifiers-}.tar.bz2"
		wget http://scisoft.fnal.gov/scisoft/packages/$packagePath >/dev/null 2>&1
		local packageFile=$( echo $packagePath | awk 'BEGIN { FS="/" } { print $NF }' )

		if [[ ! -e $packageFile ]]; then
			if [[ "$packageOs" == "slf7-x86_64" ]]; then
				# Try sl7, as they're both valid...
				detectAndPull $packageName sl7-x86_64 ${qualifiers:-"nq"} $packageVersion
			else
				echo "Unable to download $packageName"
				return 1
			fi
		else
			local returndir=$PWD
			cd $Base/products
			tar -xjf $Base/download/$packageFile
			cd $returndir
		fi
	fi
	cd $startDir
}

cd $Base/download

# 28-Feb-2017, KAB: use central products areas, if available and not skipped
# 10-Mar-2017, ELF: Re-working how this ends up in the setupARTDAQDEMO script
PRODUCTS_SET=""
if [[ $opt_skip_extra_products -eq 0 ]]; then
  FERMIOSG_ARTDAQ_DIR="/cvmfs/fermilab.opensciencegrid.org/products/artdaq"
  FERMIAPP_ARTDAQ_DIR="/grid/fermiapp/products/artdaq"
  for dir in $FERMIOSG_ARTDAQ_DIR $FERMIAPP_ARTDAQ_DIR;
  do
	# if one of these areas has already been set up, do no more
	for prodDir in $(echo ${PRODUCTS:-""} | tr ":" "\n")
	do
	  if [[ "$dir" == "$prodDir" ]]; then
		break 2
	  fi
	done
	if [[ -f $dir/setup ]]; then
	  echo "Setting up artdaq UPS area... ${dir}"
	  source $dir/setup
	  break
	fi
  done
  CENTRAL_PRODUCTS_AREA="/products"
  for dir in $CENTRAL_PRODUCTS_AREA;
  do
	# if one of these areas has already been set up, do no more
	for prodDir in $(echo ${PRODUCTS:-""} | tr ":" "\n")
	do
	  if [[ "$dir" == "$prodDir" ]]; then
		break 2
	  fi
	done
	if [[ -f $dir/setup ]]; then
	  echo "Setting up central UPS area... ${dir}"
	  source $dir/setup
	  break
	fi
  done
  PRODUCTS_SET="${PRODUCTS:-}"
fi

echo "Cloning cetpkgsupport to determine current OS"
git clone http://cdcvs.fnal.gov/projects/cetpkgsupport
os=`./cetpkgsupport/bin/get-directory-name os`

if [[ "$os" == "u14" ]]; then
	echo "-H Linux64bit+3.19-2.19" >../products/ups_OVERRIDE.`hostname`
fi

# Get all the information we'll need to decide which exact flavor of the software to install
notag=0
if [ -z "${tag:-}" ]; then 
  tag=develop;
  notag=1;
fi
if [[ -e product_deps ]]; then mv product_deps product_deps.save; fi
wget https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/repository/revisions/$tag/raw/ups/product_deps
demo_version=`grep "parent artdaq_demo" $Base/download/product_deps|awk '{print $3}'`
if [[ $notag -eq 1 ]] && [[ $opt_develop -eq 0 ]]; then
  tag=$demo_version

  # 06-Mar-2017, KAB: re-fetch the product_deps file based on the tag
  mv product_deps product_deps.orig
  wget https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/repository/revisions/$tag/raw/ups/product_deps
  demo_version=`grep "parent artdaq_demo" $Base/download/product_deps|awk '{print $3}'`
  tag=$demo_version
fi
artdaq_version=`grep "^artdaq " $Base/download/product_deps | awk '{print $2}'`
coredemo_version=`grep "^artdaq_core_demo " $Base/download/product_deps | awk '{print $2}'`
defaultQuals=`grep "defaultqual" $Base/download/product_deps|awk '{print $2}'`

defaultE=`echo $defaultQuals|cut -f1 -d:`
defaultS=`echo $defaultQuals|cut -f2 -d:`
if [ -n "${equalifier-}" ]; then 
	equalifier="e${equalifier}";
elif [ -n "${cqualifier-}" ]; then
    equalifier="c${cqualifier-}";
else
	equalifier=$defaultE
fi
if [ -n "${squalifier-}" ]; then
	squalifier="s${squalifier}"
else
	squalifier=$defaultS
fi
if [[ -n "${opt_debug:-}" ]] ; then
	build_type="debug"
else
	build_type="prof"
fi

wget http://scisoft.fnal.gov/scisoft/bundles/tools/pullProducts
chmod +x pullProducts
./pullProducts $Base/products ${os} artdaq_demo-${demo_version} ${squalifier}-${equalifier} ${build_type}
	if [ $? -ne 0 ]; then
	echo "Error in pullProducts. Please go to http://scisoft.fnal.gov/scisoft/bundles/artdaq_demo/${demo_version}/manifest and make sure that a manifest for the specified qualifiers (${squalifier}-${equalifier}) exists."
	exit 1
	fi
detectAndPull mrb noarch
export PRODUCTS=$PRODUCTS_SET
source $Base/products/setup
PRODUCTS_SET=$PRODUCTS
setup mrb
setup git
setup gitflow

export MRB_PROJECT=artdaq_demo
cd $Base
mrb newDev -f -v $demo_version -q ${equalifier}:${squalifier}:${build_type}
set +u
source $Base/localProducts_artdaq_demo_${demo_version}_${equalifier}_${squalifier}_${build_type}/setup
set -u

cd $MRB_SOURCE
if [[ $opt_develop -eq 1 ]]; then
	if [ $opt_w -gt 0 ];then
		mrb gitCheckout -d artdaq_core ssh://p-artdaq@cdcvs.fnal.gov/cvs/projects/artdaq-core
		mrb gitCheckout -d artdaq_utilities ssh://p-artdaq-utilities@cdcvs.fnal.gov/cvs/projects/artdaq-utilities
		mrb gitCheckout ssh://p-artdaq@cdcvs.fnal.gov/cvs/projects/artdaq
		mrb gitCheckout -d artdaq_core_demo ssh://p-artdaq-core-demo@cdcvs.fnal.gov/cvs/projects/artdaq-core-demo
		mrb gitCheckout -d artdaq_demo ssh://p-artdaq-demo@cdcvs.fnal.gov/cvs/projects/artdaq-demo
		mrb gitCheckout -d artdaq_mpich_plugin ssh://p-artdaq-utilities@cdcvs.fnal.gov/cvs/projects/artdaq-utilities-mpich-plugin
	else
		mrb gitCheckout -d artdaq_core http://cdcvs.fnal.gov/projects/artdaq-core
		mrb gitCheckout -d artdaq_utilities http://cdcvs.fnal.gov/projects/artdaq-utilities
		mrb gitCheckout http://cdcvs.fnal.gov/projects/artdaq
		mrb gitCheckout -d artdaq_core_demo http://cdcvs.fnal.gov/projects/artdaq-core-demo
		mrb gitCheckout -d artdaq_demo http://cdcvs.fnal.gov/projects/artdaq-demo
		mrb gitCheckout -d artdaq_mpich_plugin http://cdcvs.fnal.gov/projects/artdaq-utilities-mpich-plugin
		mrb gitCheckout -d artdaq_ganglia_plugin http://cdcvs.fnal.gov/projects/artdaq-utilities-ganglia-plugin
		mrb gitCheckout -d artdaq_epics_plugin http://cdcvs.fnal.gov/projects/artdaq-utilities-epics-plugin
		mrb gitCheckout -d artdaq_mfextensions http://cdcvs.fnal.gov/projects/mf-extensions-git
	fi
else
	if [ $opt_w -gt 0 ];then
		mrb gitCheckout -t ${coredemo_version} -d artdaq_core_demo ssh://p-artdaq-core-demo@cdcvs.fnal.gov/cvs/projects/artdaq-core-demo
		mrb gitCheckout -t ${demo_version} -d artdaq_demo ssh://p-artdaq-demo@cdcvs.fnal.gov/cvs/projects/artdaq-demo
		mrb gitCheckout -t ${artdaq_version} ssh://p-artdaq@cdcvs.fnal.gov/cvs/projects/artdaq
	else
		mrb gitCheckout -t ${coredemo_version} -d artdaq_core_demo http://cdcvs.fnal.gov/projects/artdaq-core-demo
		mrb gitCheckout -t ${demo_version} -d artdaq_demo http://cdcvs.fnal.gov/projects/artdaq-demo
		mrb gitCheckout -t ${artdaq_version} http://cdcvs.fnal.gov/projects/artdaq
	fi
fi

if [[ "x${opt_viewer-}" != "x" ]] && [[ $opt_develop -eq 1 ]]; then
	cd $MRB_SOURCE
	mrb gitCheckout -d artdaq_mfextensions http://cdcvs.fnal.gov/projects/mf-extensions-git

	qtver=$( awk '/^[[:space:]]*qt[[:space:]]*/ {print $2}' artdaq_mfextensions/ups/product_deps )

	os=`$Base/download/cetpkgsupport/bin/get-directory-name os`

	if [[ "$os" == "slf7" ]]; then
	os="sl7"
	fi

	detectAndPull qt ${os}-x86_64 ${equalifier} ${qtver}
fi


ARTDAQ_DEMO_DIR=$Base/srcs/artdaq_demo
ARTDAQ_DIR=$Base/srcs/artdaq
cd $Base
	cat >setupARTDAQDEMO <<-EOF
echo # This script is intended to be sourced.

sh -c "[ \`ps \$\$ | grep bash | wc -l\` -gt 0 ] || { echo 'Please switch to the bash shell before running the artdaq-demo.'; exit; }" || exit

if [[ -e /cvmfs/fermilab.opensciencegrid.org/products/artdaq ]]; then
  . /cvmfs/fermilab.opensciencegrid.org/products/artdaq/setup
fi

source $Base/products/setup

PRODUCTS=\`dropit -D -p"\$PRODUCTS"\`
if echo "\$PRODUCTS" | grep "$PRODUCTS_SET" >/dev/null; then
    : OK
else
    echo WARNING: PRODUCTS environment has changed from initial installation.
    echo "Product list $PRODUCTS_SET not found."
fi

setup mrb
source $Base/localProducts_artdaq_demo_${demo_version}_${equalifier}_${squalifier}_${build_type}/setup
source mrbSetEnv

if [[ "x\${ARTDAQ_MPICH_PLUGIN_DIR:-}" == "x" ]]; then
  for plugin_version in \`ups list -aK+ artdaq_mpich_plugin -q ${equalifier}:${squalifier}:eth:${build_type}|awk '{print \$2}'|sed 's/\"//g'\`;do
    if [ \`ups depend artdaq_mpich_plugin \$plugin_version -q ${equalifier}:${squalifier}:eth:${build_type} 2>/dev/null|grep -c "artdaq \$ARTDAQ_VERSION"\` -gt 0 ]; then
      setup artdaq_mpich_plugin \$plugin_version -q ${equalifier}:${squalifier}:eth:${build_type}
      break;
    fi
  done
fi

export TRACE_NAME=TRACE

export ARTDAQDEMO_REPO=$ARTDAQ_DEMO_DIR
export ARTDAQDEMO_BUILD=$MRB_BUILDDIR/artdaq_demo
#export ARTDAQDEMO_BASE_PORT=52200
export DAQ_INDATA_PATH=$ARTDAQ_DEMO_DIR/test/Generators
${opt_mfext+export ARTDAQ_MFEXTENSIONS_ENABLED=1}

export ARTDAQDEMO_DATA_DIR=${datadir}
export ARTDAQDEMO_LOG_DIR=${logdir}

export FHICL_FILE_PATH=.:\$ARTDAQ_DEMO_DIR/tools/snippets:\$ARTDAQ_DEMO_DIR/tools/fcl:\$FHICL_FILE_PATH

# 03-Jun-2018, KAB: added second call to mrbSetEnv to ensure that the code that is built
# from the srcs area in the mrb environment is what is found first in the PATH.
if [ \`echo \$ARTDAQ_DIR|grep -c "$Base"\` -eq 0 ]; then
  echo ""
  echo ">>> Setting up the MRB environment again to ensure that MRB-based executables and libraries are used during running. <<<"
  echo ""
  source mrbSetEnv
fi

alias rawEventDump="if [[ -n \\\$SETUP_TRACE ]]; then unsetup TRACE ; echo Disabling TRACE so that it will not affect rawEventDump output ; sleep 1; fi; art -c \$ARTDAQ_DIR/artdaq/ArtModules/fcl/rawEventDump.fcl"

EOF
#

# Build artdaq_demo
cd $MRB_BUILDDIR
set +u
source mrbSetEnv
set -u
export CETPKG_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))
mrb build    # VERBOSE=1
installStatus=$?

if [ $installStatus -eq 0 ]; then
	echo "artdaq-demo has been installed correctly. Please see: "
	echo "https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/wiki/Running_a_sample_artdaq-demo_system"
	echo "for instructions on how to run, or re-run this script with the --run-demo option"
	echo
	echo "Will now install DAQInterface as described at https://cdcvs.fnal.gov/redmine/projects/artdaq-utilities/wiki/Artdaq-daqinterface..."
else
	echo "BUILD ERROR!!! SOMETHING IS VERY WRONG!!!"
	echo
	echo "Skipping installation of DAQInterface"
	echo
	exit 1
fi

# Now, install DAQInterface, basically following the instructions at
# https://cdcvs.fnal.gov/redmine/projects/artdaq-utilities/wiki/Artdaq-daqinterface

daqintdir=$Base/DAQInterface

# Nov-21-2017: in order to allow for more than one DAQInterface to run
# on the system at once, we need to take it from its current HEAD of
# the develop branch, 6c15e15c0f6e06282f2fd5dd8ad478659fdb29bd

cd $Base

if [ $opt_w -gt 0 ];then
    git clone ssh://p-artdaq-utilities@cdcvs.fnal.gov/cvs/projects/artdaq-utilities-daqinterface
else
    git clone http://cdcvs.fnal.gov/projects/artdaq-utilities-daqinterface
fi
cd artdaq-utilities-daqinterface
if [[ $opt_develop -eq 1 ]]; then 
    git checkout develop
else
    # JCF, Sep-25-2018: grep out the protodune DAQInterface series when searching for the newest DAQInterface version...

    artdaq_daqinterface_version=$( git tag --sort creatordate | grep -v "v3_00_0[0-9].*" | tail -1 )
    echo "Checking out version $artdaq_daqinterface_version of artdaq_daqinterface"
    git checkout $artdaq_daqinterface_version # Fetch latest tagged version
fi

mkdir $daqintdir
cd $daqintdir
cp ../artdaq-utilities-daqinterface/bin/mock_ups_setup.sh .
cp ../artdaq-utilities-daqinterface/docs/user_sourcefile_example .
cp ../artdaq-utilities-daqinterface/docs/settings_example .
cp ../artdaq-utilities-daqinterface/docs/known_boardreaders_list_example .
cp ../artdaq-utilities-daqinterface/docs/boot.txt .

sed -i -r 's!^\s*export ARTDAQ_DAQINTERFACE_DIR.*!export ARTDAQ_DAQINTERFACE_DIR='$Base/artdaq-utilities-daqinterface'!' mock_ups_setup.sh
sed -i -r 's!^\s*export DAQINTERFACE_SETTINGS.*!export DAQINTERFACE_SETTINGS='$PWD/settings_example'!' user_sourcefile_example


# Figure out which products directory contains the xmlrpc package (for
# sending commands to DAQInterface) and set it in the settings file

productsdir=$( ups active | grep xmlrpc | awk '{print $NF}' )

if [[ -z $productsdir ]]; then
	echo "Unable to determine the products directory containing xmlrpc; will return..." >&2
	return 41
fi

sed -i -r 's!^\s*productsdir_for_bash_scripts:.*!productsdir_for_bash_scripts: '$productsdir'!' settings_example

mkdir -p $Base/run_records

sed -i -r 's!^\s*record_directory.*!record_directory: '$Base/run_records'!' settings_example

mkdir -p $Base/daqlogs
mkdir -p $Base/daqdata

sed -i -r 's!^\s*log_directory.*!log_directory: '$logdir'!' settings_example
sed -i -r 's!^\s*data_directory_override.*!data_directory_override: '$datadir'!' settings_example

sed -i -r 's!^\s*DAQ setup script:.*!DAQ setup script: '$Base'/setupARTDAQDEMO!' boot.txt

cd $Base
wget https://cdcvs.fnal.gov/redmine/projects/artdaq-demo/repository/revisions/develop/raw/tools/run_demo.sh

if [ "x${opt_run_demo-}" != "x" ]; then
	echo doing the demo

	set +u
	. ./run_demo.sh --basedir $Base --toolsdir ${Base}/srcs/artdaq_demo/tools
	set -u
fi


endtime=`date`

echo "Build start time: $starttime"
echo "Build end time:   $endtime"
