#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

node = fabricClient.DG.createNode("node")
node.addMember("foo","Scalar",3.141)
print(node.getData("foo", 0))
node.setData("foo", 0, 2.718)
print(node.getData("foo", 0))
node.resize(2)
print(node.getData("foo", 1))

fabricClient.close()
