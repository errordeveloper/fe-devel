#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

class MyStruct():
  def __init__( self, i = None, s = None ):
    if type( i ) is int and type( s ) is float:
      self.i = i
      self.s = s
    elif i is None and s is None:
      self.i = 0
      self.s = 0.0
    else:
      raise Exception( 'MyStruct: invalid use of constructor' )
    
myStructDesc = {
  'members': [ 
    { 'i': "Integer" },
    { 's': "Scalar" }
  ],
  'constructor': MyStruct
}
fabricClient.RegisteredTypesManager.registerType( "MyStruct", myStructDesc )

op = fabricClient.DependencyGraph.createOperator( "op" )
op.setEntryPoint( "entry" )
op.setSourceCode( "\n\
operator entry( io MyStruct arg[] )\n\
{\n\
  MyStruct ms;\n\
  ms.i = 42;\n\
  ms.s = 3.141;\n\
  \n\
  arg.push( ms );\n\
}\n\
" )
diagnostics = op.getDiagnostics()
if len( diagnostics ) > 0:
  for i in diagnostics:
    print( diagnostics[i].line + ": " + diagnostics[i].desc )
  print( "Full code:" )
  print( op.getFullSourceCode() )
else:
  binding = fabricClient.DG.createBinding()
  binding.setOperator(op)
  binding.setParameterLayout(["self.msaa"])
  
  node = fabricClient.DependencyGraph.createNode( "node" )
  node.addMember( "msaa", "MyStruct[]" )
  node.bindings.append(binding)
  errors = node.getErrors()
  if len( errors ) > 0:
    for i in errors:
      print( errors[i] )
  else:
    node.evaluate()
    print(fabric.stringify( node.getData("msaa", 0) ))

fabricClient.close()
