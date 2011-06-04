#!/bin/sh
# This could be a nodejs script, but it doesn't let you set ulimits, so back to
# the shell we go...

[ ! -z "PROC_ULIMIT_FILES" ] && ulimit -n $PROC_ULIMIT_FILES > /dev/null 2>&1
[ ! -z "PROC_DIRECTORY" ] && cd $PROC_DIRECTORY

if [ ! -z "$PROC_USER" -a -z "$PROC_USER_SWITCHED_OK" ] ; then
  # PROC_USER could be a uid, setuidgid doesn't like those.
  # Ensure it's a username.
  username=$(getent passwd $PROC_USER | awk -F: '{print $1}')
  exec env PROC_USER_SWITCHED_OK=1 setuidgid $username $0 "$@"
  #exec env PROC_USER_SWITCHED_OK=1 su -m - $PROC_USER $0 "$@" 
  echo "Something bad happened, maybe we couldn't find 'env' or 'setuidgid' ?"
  exit 255
fi

if [ ! -z "$PROC_NICE" -a -z "$PROC_NICE_SWITCHED_OK" ] ; then
  exec env PROC_NICE_SWITCHED_OK=1 nice -n $PROC_NICE $0 "$@"
  echo "Something bad happened, maybe we couldn't find 'env' or 'nice' ?"
  exit 255
fi

#ulimit -n
#id
#pwd
exec "$@"
