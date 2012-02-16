#!/bin/sh

TEST_SUFFIX="py"
TEST_CMD="python -u"

. ../runtests.sh

if [ -d TMP ]; then
	rm -r TMP
fi

exit $ERROR

