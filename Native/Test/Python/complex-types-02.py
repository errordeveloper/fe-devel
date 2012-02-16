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
op.setEntryFunctionName("entry")
op.setSourceCode("\n\
operator entry( io MyStruct arg<>[] )\n\
{\n\
  report 'parent data: ' + arg;\n\
}\n\
")
diagnostics = op.getDiagnostics()
if len( diagnostics ) > 0:
  for i in diagnostics:
    print( diagnostics[i].line + ": " + diagnostics[i].desc )
  print( "Full code:" )
  print( op.getFullSourceCode() )
else:
  parentNode = fabricClient.DependencyGraph.createNode( "parentNode" )
  parentNode.addMember( "msa", "MyStruct[]" )
  parentNode.resize( 2 )
  parentNode.setData( "msa", 0, [MyStruct( 42, 3.141 ), MyStruct( 64, 5.67 ) ] )
  parentNode.setData( "msa", 1, [MyStruct( 7, 2.718 )] )
  print(fabric.stringify( parentNode.getData("msa", 0) ))
  print(fabric.stringify( parentNode.getData("msa", 1) ))

  binding = fabricClient.DG.createBinding()
  binding.setOperator(op)
  binding.setParameterLayout(["parent.msa<>"])

  node = fabricClient.DependencyGraph.createNode( "node" )
  node.setDependency( parentNode, "parent" )
  node.bindings.append(binding)
  errors = node.getErrors()
  if len( errors ) > 0:
    for i in errors:
      print( errors[i] )
  else:
    node.evaluate()

fabricClient.close()
