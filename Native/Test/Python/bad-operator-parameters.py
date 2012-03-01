#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()
            
op = fabricClient.DependencyGraph.createOperator("bar")
op.setEntryFunctionName("foo")
op.setSourceCode("operator foo( Boolean testBool, io Integer foo ) { if(testBool){ foo = 10; } }")

binding = fabricClient.DependencyGraph.createBinding()
binding.setOperator( op )
binding.setParameterLayout( [ "self.testBool", "self.foo" ] )

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( "testBool", "Boolean" )
node.addMember( "foo", "Integer" )
node.bindings.append(binding)

errors = node.getErrors()
print len( errors )
print errors[0]

fabricClient.close()
