#!/bin/bash

LIB="libinsight.so.1"
DAEMON="insightd"

if [ -r ../libinsight/$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}../libinsight"
elif [ -r ./$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}."
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

if [ -x ../insightd/$DAEMON ]; then
	export PATH="../insightd:$PATH"
fi

exec ./insight "$@"
