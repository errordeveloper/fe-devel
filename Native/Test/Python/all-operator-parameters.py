#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

for i in range( 1, 32 ):
  klCode = "operator foo( "
  for j in range( 0, i ):
    if j > 0: klCode += ", "
    klCode += "io Integer v" + str(j)
  klCode += " ) { "
  for j in range( 0, i ):
    klCode += "v" + str(j) + " -= " + str(j+1) + "; "
  klCode += " }"
  
  op = fabricClient.DependencyGraph.createOperator("foo_"+str(i))
  op.setEntryFunctionName("foo")
  op.setSourceCode(klCode)

  node = fabricClient.DependencyGraph.createNode("bar_"+str(i))

  layout = []
  for j in range( 0, i ):
    layout.append( "self.p" + str(j) )
    node.addMember( "p" + str(j), "Integer" )
    node.setData( "p" + str(j), 0, j+1 )

  binding = fabricClient.DG.createBinding()
  binding.setOperator(op)
  binding.setParameterLayout(layout)
  
  node.bindings.append(binding)
  node.evaluate()
 
  for j in range( 0, i ):
    if node.getData( "p" + str(j), 0 ) != 0:
      print( "Parameter " + str(j) + " fails when evaluating " + str(i) + " params" )

fabricClient.close()


