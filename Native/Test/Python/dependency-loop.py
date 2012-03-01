#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

node1 = fabricClient.DependencyGraph.createNode("one")
node2 = fabricClient.DependencyGraph.createNode("two")

try:
  node1.setDependency( node1, "reflex" )
  fabricClient.flush()
except Exception as e:
  print e

try:
  node1.setDependency( node2, "one" )
  node2.setDependency( node1, "two" )
  fabricClient.flush()
except Exception as e:
  print e

fabricClient.close()
