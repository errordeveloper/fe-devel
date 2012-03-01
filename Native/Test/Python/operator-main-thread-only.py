#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

o = F.DG.createOperator("op")
o.setSourceCode("operator entry( io Integer foo ) { report 'Hello'; }")
o.setEntryFunctionName("entry")
print(o.getMainThreadOnly())
o.setMainThreadOnly(True)
print(o.getMainThreadOnly())

b = F.DG.createBinding()
b.setOperator(o)
b.setParameterLayout(["self.foo"])

n = F.DG.createNode("foo")
n.addMember("foo", "Integer")
n.resize(64)
n.bindings.append(b)
n.evaluate()

F.close()
