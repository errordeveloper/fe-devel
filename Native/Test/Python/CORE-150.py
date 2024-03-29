#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

op = fabricClient.DependencyGraph.createOperator( "op" )
op.setEntryPoint('entry')
op.setSourceCode("operator entry( io Scalar input[][] ) {}")

binding = fabricClient.DependencyGraph.createBinding()
binding.setOperator( op )
binding.setParameterLayout( [ "self.input" ] )

node = fabricClient.DependencyGraph.createNode( "node" )
node.addMember( "input", "Scalar[]" )
node.bindings.append( binding )
node.setData( "input", [17] )
print( node.getErrors() )

fabricClient.close()
