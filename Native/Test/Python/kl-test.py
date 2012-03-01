#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

parentOp = fabricClient.DependencyGraph.createOperator( "parentOp" )
parentOp.setEntryFunctionName('entry')
parentOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }")

parentBinding = fabricClient.DependencyGraph.createBinding()
parentBinding.setOperator(parentOp)
parentBinding.setParameterLayout([ "self.input", "self.output" ])

parentNode = fabricClient.DependencyGraph.createNode( "parent" )
parentNode.addMember( "input", "Scalar" )
parentNode.setData( "input", 0, 14 )
parentNode.addMember( "output", "Scalar" )
parentNode.bindings.append(parentBinding)

childOp = fabricClient.DependencyGraph.createOperator( "childOp" )
childOp.setEntryFunctionName('entry')
childOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }")

childBinding = fabricClient.DependencyGraph.createBinding()
childBinding.setOperator(childOp)
childBinding.setParameterLayout([ "parent.output", "self.output" ])

childNode = fabricClient.DependencyGraph.createNode( "child" )
childNode.setDependency( parentNode, "parent" )
childNode.addMember( "output", "Scalar" )
childNode.bindings.append(childBinding)

childNode.evaluate()
print( childNode.getData( 'output', 0 ) )

fabricClient.close()
