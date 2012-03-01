#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
FABRIC = fabric.createClient()

class Vec3():
  def __init__( self, x = None, y = None, z = None ):
    if type( x ) is int and type( y ) is int and type( z ) is int:
      self.x = x
      self.y = y
      self.z = z
    elif x is None and y is None and z is None:
      self.x = 0
      self.y = 0
      self.z = 0
    else:
      raise Exception( 'new Vec3: invalid arguments' )

desc = {
  'members': [ { 'x':'Scalar' }, { 'y':'Scalar' }, { 'z':'Scalar' } ],
  'constructor': Vec3,
  'kBindings': "\
function Vec3( Scalar x, Scalar y, Scalar z )\n\
{\n\
  this.x = x;\n\
  this.y = y;\n\
  this.z = z;\n\
}\n\
"
}

FABRIC.RegisteredTypesManager.registerType( 'Vec3', desc )

event = FABRIC.DependencyGraph.createEvent("event")
eventHandler = FABRIC.DependencyGraph.createEventHandler("eventHandler")
event.appendEventHandler( eventHandler )
node = FABRIC.DependencyGraph.createNode( "node" )
eventHandler.setScope( 'self', node )

nodeOne = FABRIC.DependencyGraph.createNode( "nodeOne" )

operatorOne = FABRIC.DependencyGraph.createOperator( "operatorOne" )
operatorOne.setEntryFunctionName('test')
operatorOne.setSourceCode("operator test( io Boolean select, io Vec3 value ) { select = true; value.x = 7; }")

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
operatorTwo.setSourceCode("operator test( io Boolean select, io Vec3 value ) { value.x = 4; }")
bindingTwo = FABRIC.DependencyGraph.createBinding()
bindingTwo.setOperator( operatorTwo )
bindingTwo.setParameterLayout( [] )
eventHandlerTwo = FABRIC.DependencyGraph.createEventHandler("eventHandlerTwo")
eventHandlerTwo.setScope( 'self', nodeTwo )
eventHandlerTwo.setSelector( 'self', bindingTwo )
eventHandler.appendChildEventHandler( eventHandlerTwo )

event.setSelectType( 'Vec3' )
result = event.select()

if len( result ) != 1:
  print( "incorrect number of results" )

if result[0]['value'].x != 7:
  print( "incorrect value of results" )

FABRIC.close()
