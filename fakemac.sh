#!/bin/sh

if test $# -lt 2
then
	echo "usage: $0 macaddress command ..." >&2
	exit 1
fi
export MAC_ADDRESS="$1"
shift

LD_PRELOAD=/usr/lib/libfakemac.so "$@"
