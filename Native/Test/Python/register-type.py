import json
import fabric
fabricClient = fabric.createClient()

class Vec2():
  def __init__( self, x = None, y = None ):
    if type( x ) is float and type( y ) is float:
      self.x = x
      self.y = y
    elif x is None and y is None:
      self.x = 0
      self.y = 0
    else:
      raise Exception( 'Vec2: invalid arguments' )

  def sum( self ):
    return self.x + self.y

desc = {
  'members': [ { 'x':'Scalar' }, { 'y':'Scalar' } ],
  'constructor': Vec2,
  'klBindings': {
    'filename': "(inline)",
    'sourceCode': "\
function Vec2(Scalar x, Scalar y)\n\
{\n\
  this.x = x;\n\
  this.y = y;\n\
}\n\
"
  }
}

fabricClient.RegisteredTypesManager.registerType( 'Vec2', desc )
print(json.dumps(fabricClient.RT.getRegisteredTypes()['Vec2']))

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( 'vec2', 'Vec2' )
node.setData( 'vec2', 0, Vec2( 5.6, 4.3 ) )
data = node.getData("vec2", 0)
print( json.dumps( fabric.typeToDict( data ) ) )
print( data.sum() )

op = fabricClient.DG.createOperator("op")
op.setSourceCode("(inline)", "use Vec2; operator entry( io Vec2 vec2 ) { vec2 = Vec2(8.9, 2.3); }")
op.setEntryFunctionName("entry")
if len( op.getErrors() ):
  print( "ERRORS: " + str( op.getErrors() ) )
  if op.getDiagnostics() is not None:
    print( "DIAGNOSTICS: " + str( op.getDiagnostics() ) )

binding = fabricClient.DG.createBinding()
binding.setOperator(op)
binding.setParameterLayout(["self.vec2"])

node.bindings.append(binding)
node.evaluate()
data = node.getData("vec2", 0)
print( json.dumps( fabric.typeToDict( data ) ) )
print(data.sum())

class ComplexType():
  def __init__( self ):
    vec2 = Vec2( 3.14, 2.71 )
    a2_vec2 = [ vec2, vec2 ]
    a_a2_vec2 = [ a2_vec2, a2_vec2, a2_vec2 ]
    a2_a_a2_vec2 = [ a_a2_vec2, a_a2_vec2 ]
    a_a2_a_a2_vec2 = [ a2_a_a2_vec2 ]
    self.member = a_a2_a_a2_vec2

complexTypeDesc = {
  'members': [ { 'member': 'Vec2[][2][][2]' } ],
  'constructor': ComplexType
}

fabricClient.RT.registerType('ComplexType', complexTypeDesc)
print(json.dumps(fabric.typeToDict(fabricClient.RT.getRegisteredTypes()['ComplexType'][ 'defaultValue' ])))
node.addMember( 'ct', 'ComplexType' )
print(json.dumps(fabric.typeToDict( node.getData( 'ct', 0 ) ) ) )

fabricClient.close()
