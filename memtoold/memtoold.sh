#!/bin/bash

LIB="libmemtool.so.1"
BASE="$(dirname $0)"

if [ -r "$BASE/../libmemtool/$LIB" ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}$BASE/../libmemtool"
elif [ -r "$BASE/$LIB" ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}$BASE"
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

exec "$BASE/memtoold" "$@"
