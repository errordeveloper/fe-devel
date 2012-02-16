#
# Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
#

.PHONY: default xcode test

default:
	scons -k

xcode:
	PYTHONUNBUFFERED=1 scons -k

test: build
	$(MAKE) -C Test
