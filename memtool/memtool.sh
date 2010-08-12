#!/bin/bash

LIB="libmemtool.so.1"
DAEMON="memtoold"

if [ -r ../libmemtool/$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}../libmemtool"
elif [ -r ./$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}."
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

if [ -x ../memtoold/$DAEMON ]; then
	export PATH="../memtoold:$PATH"
fi

exec ./memtool "$@"
