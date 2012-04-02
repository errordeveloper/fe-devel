import fabric

def run( opts ):
  c = fabric.createClient( opts )
  pn = c.DG.createNode( "parent" )
  pn.addMember( "input", "String" )
  pn.addMember( "output", "String" )

  pn.setData( "input", 0, "input1" )
  print "parent input: " + str( pn.getData( "input", 0 ) )

  pop = c.DG.createOperator( "parentOp" )
  pop.setEntryPoint( "parentOp" )
  pop.setSourceCode( "operator parentOp( String input, io String output ) { output = 'parent: ' + input; }" )

  pb = c.DG.createBinding()
  pb.setOperator( pop )
  pb.setParameterLayout( [ "self.input", "self.output" ] )

  pn.bindings.append( pb )

  n = c.DG.createNode( "child" )
  n.addMember( "output", "String" )
  n.setDependency( pn, 'parent' )

  op = c.DG.createOperator( "childOp" )
  op.setEntryPoint( "childOp" )
  op.setSourceCode( "operator childOp( io String input, io String output ) { output = 'child: ' + input; }" )

  b = c.DG.createBinding()
  b.setOperator( op )
  b.setParameterLayout( [ "parent.output", "self.output" ] )
  n.bindings.append( b )

  n.evaluate()
  print "child output: " + n.getData( "output", 0 )
  c.close()

run( None )
run( { 'logWarnings': True } )
run( 'foobar' )
run( { 'logWarnings': 'yes' } )


