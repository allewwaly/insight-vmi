#!/bin/bash

export LD_LIBRARY_PATH="../libinsight:$LD_LIBRARY_PATH"
DIR=$(dirname $0)

for t in $DIR/*/test_*; do
	if [ -x "$t" ]; then
		echo "########## Running: $t ##########"
		$t
		echo ""
	fi
done

