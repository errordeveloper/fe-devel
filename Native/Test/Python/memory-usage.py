#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

n = F.DG.createNode("foo")
n.addMember("bar", "String[]")
print(F.getMemoryUsage())

F.close()
