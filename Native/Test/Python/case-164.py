#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

operator = F.DG.createOperator("op")
operator.setEntryFunctionName("entry")
operator.setSourceCode("\
operator entry( io Size foo ) {\n\
  report 'foo = ' + foo;\n\
}\n\
")

if len( operator.getDiagnostics() ) > 0:
  print(operator.getDiagnostics())

binding = F.DependencyGraph.createBinding()
binding.setOperator(operator)
binding.setParameterLayout(['self.bar','self.foo'])

node = F.DG.createNode("node")
node.addMember('bar', 'Size')
node.setData('bar', 0, 42)
node.bindings.append(binding)
if len( node.getErrors() ) > 0:
  print(node.getErrors())
try:
  node.evaluate()
except Exception as e:
  print(e)

F.close()
