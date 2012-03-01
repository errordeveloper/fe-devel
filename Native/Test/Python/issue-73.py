#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

# test that setting slice count to 'None' gives exception
# and not a crash

import fabric, sys
F = fabric.createClient()
node = F.DG.createNode("foo")
try:
  node.resize(None)
  F.flush()
except Exception as e:
  print "Caught exception:", e
node.evaluate()
F.close()
