#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

node = fabricClient.DependencyGraph.createNode( "node" )
node.addMember( "input", "Scalar" )
node.setData( "input", 17 )

op = fabricClient.DependencyGraph.createOperator( "op" )
op.setEntryFunctionName('entry')
op.setSourceCode("operator entry( io Scalar input ) {}")

binding = fabricClient.DependencyGraph.createBinding()
binding.setOperator( op )
binding.setParameterLayout( [ "node.doesntExist" ] )

eh1 = fabricClient.DependencyGraph.createEventHandler("eventHandlerOne")
eh1.preDescendBindings.append(binding)
eh1.setScope( 'node', node )

eh2 = fabricClient.DependencyGraph.createEventHandler("eventHandlerTwo")

e = fabricClient.DependencyGraph.createEvent("event")
e.appendEventHandler( eh1 )
e.appendEventHandler( eh2 )

print( "Errors on e:" )
print( e.getErrors() )

fabricClient.close()
