import fabric

c = fabric.createClient()

pn = c.DG.createNode( "parent" )
pn.addMember( "input", "String" )
pn.addMember( "output", "String" )

size = 2
pn.resize( size )
for i in range( 0, size ):
  pn.setData( "input", i, "input"+str(i) )
  print "parent input: " + str( pn.getData( "input", i ) )

pop = c.DG.createOperator( "parentOp" )
pop.setEntryPoint( "parentOp" )
pop.setSourceCode( "operator parentOp( String input, io String output ) { output = 'parent: ' + input; report 'parentOp: ' + input; }" )

pb = c.DG.createBinding()
pb.setOperator( pop )
pb.setParameterLayout( [ "self.input", "self.output" ] )

pn.bindings.append( pb )

n = c.DG.createNode( "child" )
n.addMember( "output", "String" )
n.resize( 3 )
n.setDependency( pn, 'parent' )

# test normal parameters
op = c.DG.createOperator( "childOp" )
op.setEntryPoint( "childOp" )
op.setSourceCode( "operator childOp( String input, io String output ) { output = 'child: ' + input; report 'childOp: '+input; }" )

b = c.DG.createBinding()
b.setOperator( op )
b.setParameterLayout( [ "parent.output", "self.output" ] )
n.bindings.append( b )

print n.getErrors()

c.close()

