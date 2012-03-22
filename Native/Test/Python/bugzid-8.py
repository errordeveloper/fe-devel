#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

o = F.DG.createOperator("op")
o.setSourceCode("\
operator entry(io Size a, io String b) {\n\
  report 'Hello ' + a + ' ' + b;\n\
}\n\
")
o.setEntryPoint("entry")

b = F.DG.createBinding()
b.setOperator(o)
b.setParameterLayout([
  "self.foo",
  "self.bar"
])

n = F.DG.createNode("node")
n.addMember("foo", "String")
n.addMember("bar", "Size")
n.bindings.append(b)
print(fabric.stringify(n.getErrors()))

F.close()
