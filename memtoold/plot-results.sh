#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <file1> [<file2> ...]"
fi


PLOT=""
while [ -n "$1" ]; do
	if [ -n "$PLOT" ]; then
		PLOT="$PLOT, "
	fi
	PLOT="${PLOT}'$1' using 10 title '$(basename $1)'"
	shift
done


exec gnuplot -e "set key outside above; set xrange [-0.5:]; set xtics mirror 5; set mxtics 5; set style fill solid; set style data histogram; set style histogram clustered gap 1; set bar 1.0; plot $PLOT; pause -1;"
