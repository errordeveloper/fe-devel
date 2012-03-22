#
# Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
#

.PHONY: default cli xcode test

default:
	scons -k

cli:
	scons -k cli

xcode:
	$(MAKE) -C Native xcode

test: cli
	$(MAKE) -C Native test
