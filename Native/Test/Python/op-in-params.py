import fabric

c = fabric.createClient()

n = c.DG.createNode("test")
n.addMember( "input", "Float32" )
n.addMember( "output", "String" )

op = c.DG.createOperator( "testOp" )
op.setEntryPoint( "entry" )
op.setSourceCode( "operator entry( Float32 input, io String output ) { input = 1.2; output = 'myoutput: ' + input; }" )
print op.getErrors()
print op.getDiagnostics()

op.setSourceCode( "operator entry( Float32 input, io String output ) { output = 'myoutput: ' + input; }" )

b = c.DG.createBinding()
b.setOperator( op )
b.setParameterLayout( [ "self.input", "self.output" ] )

n.bindings.append( b )

n.setData( "input", 0, 5.2 )
print "input: " + str( n.getData( "input", 0 ) )

n.evaluate()

print "input: " + str( n.getData( "input", 0 ) )
print "output: " + n.getData( "output", 0 )

c.close()

