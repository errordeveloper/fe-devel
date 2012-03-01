#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

FABRIC = fabric.createClient()

event = FABRIC.DependencyGraph.createEvent("event")
eventHandler = FABRIC.DependencyGraph.createEventHandler("eventHandler")
event.appendEventHandler( eventHandler )
node = FABRIC.DependencyGraph.createNode( "node" )
eventHandler.setScope( 'self', node )

nodeOne = FABRIC.DependencyGraph.createNode( "nodeOne" )
operatorOne = FABRIC.DependencyGraph.createOperator( "operatorOne" )
operatorOne.setEntryFunctionName('test')
operatorOne.setSourceCode("operator test( io Boolean select, io Scalar value ) { select = true; value = 7; }")
bindingOne = FABRIC.DependencyGraph.createBinding()
bindingOne.setOperator( operatorOne )
bindingOne.setParameterLayout( [] )
eventHandlerOne = FABRIC.DependencyGraph.createEventHandler("eventHandlerOne")
eventHandlerOne.setScope( 'self', nodeOne )
eventHandlerOne.setSelector( 'self', bindingOne )
eventHandler.appendChildEventHandler( eventHandlerOne )

nodeTwo = FABRIC.DependencyGraph.createNode( "nodeTwo" )
operatorTwo = FABRIC.DependencyGraph.createOperator( "operatorTwo" )
operatorTwo.setEntryFunctionName('test')
operatorTwo.setSourceCode("operator test( io Boolean select, io Scalar value ) { value = 4; }")
bindingTwo = FABRIC.DependencyGraph.createBinding()
bindingTwo.setOperator( operatorTwo )
bindingTwo.setParameterLayout( [] )
eventHandlerTwo = FABRIC.DependencyGraph.createEventHandler("eventHandlerTwo")
eventHandlerTwo.setScope( 'self', nodeTwo )
eventHandlerTwo.setSelector( 'self', bindingTwo )
eventHandler.appendChildEventHandler( eventHandlerTwo )
         
event.setSelectType( 'Scalar' )
result = event.select()

if len( result ) != 1:
  print( "incorrect result length" )

if result[0]['value'] != 7:
  print( "incorrect result value" )

FABRIC.close()
