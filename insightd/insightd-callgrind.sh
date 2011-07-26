#!/bin/bash

LIB="libinsight.so.1"


if [ -r ../libinsight/$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}../libinsight"
elif [ -r ./$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}."
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

exec valgrind --tool=callgrind ./insightd "$@"
