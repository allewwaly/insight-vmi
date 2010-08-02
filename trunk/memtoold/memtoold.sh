#!/bin/bash

LIB="libmemtool.so.1"


if [ -d ../libmemtool/$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}../libmemtool"
elif [ -r ./$LIB ]; then
	export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+LD_LIBRARY_PATH:}."
else
	echo "Cannot find $LIB, aborting." >&2
	exit 1
fi

exec ./memtoold "$@"
