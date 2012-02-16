#!/bin/sh

TEST_SUFFIX="kl"
TEST_CMD="../../build/$FABRIC_BUILD_OS/$FABRIC_BUILD_ARCH/$FABRIC_BUILD_TYPE/Fabric/Tools/KL/kl --run"

. ../runtests.sh

exit $ERROR

