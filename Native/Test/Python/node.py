#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

def mapNamedObjectsToNames( objects ):
  result = {}
  for i in objects:
    result[ i ] = objects[ i ].getName()
  return result

node = F.DG.createNode("node")
print(node.getType())
node.addMember("foo", "String")
print(node.getMembers())
node.resize(2)
print(node.size())
node.setData("foo", 0, "fred")
node.setData("foo", 1, "joe")
print(node.getData("foo", 0))
print(node.getData("foo", 1))

node2 = F.DG.createNode("node2")
node.setDependency( node2, "parent" )
print(mapNamedObjectsToNames(node.getDependencies()))

F.close()
