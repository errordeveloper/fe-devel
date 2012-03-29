import fabric

c = fabric.createClient()

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
n.resize( 2 )
n.setDependency( pn, 'parent' )

# test normal parameters
op = c.DG.createOperator( "childOp" )
op.setEntryPoint( "childOp" )
op.setSourceCode( "operator childOp( io String input, io String output ) { output = 'child: ' + input; }" )

b = c.DG.createBinding()
b.setOperator( op )
b.setParameterLayout( [ "parent.output", "self.output" ] )
n.bindings.append( b )

# test container parameter
op2 = c.DG.createOperator( "childOp2" )
op2.setEntryPoint( "childOp2" )
op2.setSourceCode( "operator childOp2( io Container parent ) { }" )

b2 = c.DG.createBinding()
b2.setOperator( op2 )
b2.setParameterLayout( [ "parent" ] )
n.bindings.append( b2 )

# test sliced array parameter
op3 = c.DG.createOperator( "childOp3" )
op3.setEntryPoint( "childOp3" )
op3.setSourceCode( "operator childOp3( io String input<>, io String output<> ) { for (Size i=0; i<input.size; i++) { output[i] = 'child: ' + input[i]; } }" )

b3 = c.DG.createBinding()
b3.setOperator( op3 )
b3.setParameterLayout( [ "parent.output<>", "self.output<>" ] )
n.bindings.append( b3 )

n.evaluate()

print "child output: " + n.getData( "output", 0 )

c.close()

