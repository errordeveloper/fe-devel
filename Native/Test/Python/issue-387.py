#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

opOne = fabricClient.DependencyGraph.createOperator( "opOne" )
opOne.setEntryPoint('entry')
opOne.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }")

bindingOne = fabricClient.DependencyGraph.createBinding()
bindingOne.setOperator( opOne )
bindingOne.setParameterLayout( [ "self.input", "self.output" ] )

opTwo = fabricClient.DependencyGraph.createOperator( "opTwo" )
opTwo.setEntryPoint('entry')
opTwo.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 3 * input; }")

bindingTwo = fabricClient.DependencyGraph.createBinding()
bindingTwo.setOperator( opTwo )
bindingTwo.setParameterLayout( [ "self.input", "self.output" ] )

node = fabricClient.DependencyGraph.createNode( "parent" )
node.addMember( "input", "Scalar" )
node.addMember( "output", "Scalar" )
node.resize( 2 )
node.setData( 'input', 0, 3 )
node.setData( 'input', 1, 7 )
node.setData( 'output', 0, 0 )
node.setData( 'output', 1, 0 )
node.evaluate()
print(fabric.stringify(node.getBulkData()))
print("bindings.length = " + str(node.bindings.getLength()))

node.setData( 'output', 0, 0 )
node.setData( 'output', 1, 0 )
node.bindings.append( bindingOne )
node.evaluate()
print(fabric.stringify(node.getBulkData()))
print("bindings.length = " + str(node.bindings.getLength()))

node.setData( 'output', 0, 0 )
node.setData( 'output', 1, 0 )
node.bindings.remove(0)
node.evaluate()
print(fabric.stringify(node.getBulkData()))
print("bindings.length = " + str(node.bindings.getLength()))

node.setData( 'output', 0, 0 )
node.setData( 'output', 1, 0 )
node.bindings.append(bindingTwo)
node.evaluate()
print(fabric.stringify(node.getBulkData()))
print("bindings.length = " + str(node.bindings.getLength()))

node.setData( 'output', 0, 0 )
node.setData( 'output', 1, 0 )
node.bindings.remove(0)
node.evaluate()
print(fabric.stringify(node.getBulkData()))
print("bindings.length = " + str(node.bindings.getLength()))

fabricClient.close()
