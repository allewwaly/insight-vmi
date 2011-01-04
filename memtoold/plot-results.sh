#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: $0 <file1> [<file2> ...]"
	exit 0
fi


PLOT1=""
PLOT2=""
COUNT=0

while [ -n "$1" ]; do
	if [ -n "$PLOT1" ]; then
		PLOT1="$PLOT1, "
		PLOT2="$PLOT2, "
	fi
	FILE=$(basename $1)
	
	PLOT1="${PLOT1}'$1' using 10:xticlabels(sprintf(\"%d-%d\", \$1, \$3)) title '${FILE%.result}'"
	PLOT2="${PLOT2}'$1' using 14:xticlabels(12) title '${FILE%.result}'"

	((COUNT++))
	shift
done


exec gnuplot -e "
set term wxt size 1900, 800;
set xrange [-0.5:];
set style fill solid;
set style data histogram;
set style histogram clustered gap $((COUNT/2 + 1));
set bar 1.0;

set size 1.0, 1.0;
set origin 0.0, 0.0;
set multiplot;

set size 1.0, 0.4;
set origin 0.0, 0.0;
unset key;
set xtics mirror 5 rotate by 90 offset 0, -1;
plot $PLOT2;

set size 1.0, 0.6;
set origin 0.0, 0.4;
set key outside above;
set xtics mirror 5 rotate by 90 offset 0, -2;
plot $PLOT1;


unset multiplot;
pause -1;"
