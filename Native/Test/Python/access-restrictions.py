import fabric
fabricClient = fabric.createClient()

op = fabricClient.DependencyGraph.createOperator("foo")
op.setEntryFunctionName("entry")
op.setSourceCode("\
operator entry( io Integer element, io Integer slice[] )\n\
{\n\
}\n\
")

binding = fabricClient.DG.createBinding()
binding.setOperator(op)
binding.setParameterLayout(["self.x", "self.x[]"])

node = fabricClient.DependencyGraph.createNode( "bar" )
node.addMember( "x", "Integer" )
node.resize( 2 )
node.bindings.append(binding)

fabricClient.close()
