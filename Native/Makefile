#
# Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
#

.PHONY: default xcode test

default:
	$(MAKE) -C .. default

xcode:
	PYTHONUNBUFFERED=1 scons -k

test:
	$(MAKE) -C Test
