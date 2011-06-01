#!/bin/sh
# This could be a nodejs script, but it doesn't let you set ulimits, so back to
# the shell we go...

[ ! -z "PROC_ULIMIT_FILES" ] && ulimit -n $PROC_ULIMIT_FILES
[ ! -z "PROC_DIRECTORY" ] && cd $PROC_DIRECTORY

if [ ! -z "$PROC_USER" -a -z "$PROC_USER_SWITCHED_OK" ] ; then
  #exec env PROC_USER_SWITCHED_OK=1 setuidgid $PROC_USER sh $0 "$@"
  exec env PROC_USER_SWITCHED_OK=1 su -m - $PROC_USER $0 "$@" 
  exit
fi

#ulimit -n
#id
#pwd
exec nice -n $PROC_NICE "$@"
