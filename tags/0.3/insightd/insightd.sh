#!/bin/bash

LIB="libinsight.so.1"
BASE="$(dirname $0)"

if [ -r "$BASE/../libinsight/$LIB" ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}$BASE/../libinsight"
elif [ -r "$BASE/$LIB" ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}$BASE"
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

exec "$BASE/insightd" "$@"
