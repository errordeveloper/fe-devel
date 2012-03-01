#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

FABRIC = fabric.createClient()

def reportErrors( obj, expected ):
  numErrors = len( obj.getErrors() )
  if numErrors != expected:
    print( obj.getName() + " errors = " + str( numErrors ) +
      ", expected " + str( expected ) )

event = FABRIC.DependencyGraph.createEvent("bar")
event.fire()

reportErrors( event, 0 )

eventHandler = FABRIC.DependencyGraph.createEventHandler("baz")
event.appendEventHandler( eventHandler )
event.fire()

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )

node = FABRIC.DependencyGraph.createNode("foo")
eventHandler.setScope( "self", node )
event.fire()

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )
reportErrors( node, 0 )

op = FABRIC.DependencyGraph.createOperator("fooTwo")
binding = FABRIC.DependencyGraph.createBinding()
binding.setOperator( op )
binding.setParameterLayout( [ "self.foo" ] )
node.bindings.append( binding )

try:
  event.fire()
except Exception:
  print( "create and push operator: fire event successfully fails" )

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )
reportErrors( node, 3 )

op.setEntryFunctionName("foo")
op.setSourceCode("operator foo( io Integer bar ) { bar = 7; }")

try:
  event.fire()
except Exception:
  print( "load operator bytecode: fire event successfully fails" )

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )
reportErrors( node, 1 )

node.addMember( "foo", "Integer" )
event.fire()

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )
reportErrors( node, 0 )

if node.getData('foo') != 7:
  print( "node.getData('foo') != 7" )

node.removeMember( "foo" )
try:
  event.fire()
except Exception:
  print( "remove node member: fire event successfully fails" )

reportErrors( event, 0 )
reportErrors( eventHandler, 0 )
reportErrors( node, 1 )

FABRIC.close()
