import fabric

def run( opts ):
  c = fabric.createClient( opts )

  n = c.DG.createNode( "foo" )
  n.addMember( "arr", "Size[]" )
  n.setData( "arr", 0, [ 1 ] )

  op = c.DG.createOperator( "op" )
  op.setSourceCode( "operator entry(Size arr[]) { arr[1]; } " )
  op.setEntryPoint( "entry" )

  b = c.DG.createBinding()
  b.setOperator( op )
  b.setParameterLayout( [ "self.arr" ] )

  n.bindings.append( b )

  try:
    n.evaluate()
  except Exception as e:
    print e

  c.close()

run( { 'guarded': True } )
run( None )

