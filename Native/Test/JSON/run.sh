#!/bin/sh

TEST_SUFFIX="json"
TEST_CMD="../../../build/Native/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE/Fabric/Tools/ParseJSON/parseJSON"

. ../runtests.sh

exit $ERROR

