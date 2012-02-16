#!/bin/sh

TEST_SUFFIX="js"
TEST_CMD="node"

. ../runtests.sh

if [ -d TMP ]; then
	rm -r TMP
fi

exit $ERROR

