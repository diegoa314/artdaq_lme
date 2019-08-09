#!/bin/bash

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` -q <qualifiers>
examples: `basename $0` -q s64:e15:prof
-q        Qualifier set to setup
"

if [[ "x$ARTDAQ_VERSION" == "x" ]];then
	echo "You must first setup artdaq!"
	return 1
fi

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= quals=
while [ -n "${1-}" ];do
	if expr "x${1-}" : 'x-' >/dev/null;then
		op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
		leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
		test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
		case "$op" in
			\?*|h*)     eval $op1chr; do_help=1;;
			q*)         eval $op1arg; quals=$1; shift;;
			*)          echo "Unknown option -$op"; do_help=1;;
		esac
	else
		aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
	fi
done
eval "set -- $args \"\$@\""; unset args aa
set -u   # complain about uninitialed shell variables - helps development

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit

for plugin_version in `ups list -aK+ artdaq_mpich_plugin -q $quals|awk '{print $2}'`;do
	if [ `ups depend artdaq_mpich_plugin $plugin_version -q $quals|grep -c "artdaq $ARTDAQ_VERSION"` -gt 0 ]; then
	  setup artdaq_mpich_plugin $plugin_version -q $quals
	  return;
	fi
done