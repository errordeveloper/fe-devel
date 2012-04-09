/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();
            
op = FABRIC.DependencyGraph.createOperator("bar");
op.setEntryPoint("foo");
op.setSourceCode("operator foo( Boolean testBool, io Integer foo ) { if(testBool){ foo = 10; } }");

binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( [ "self.testBool", "self.foo" ] );
console.log(binding.getParameterLayout());

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "testBool", "Boolean" );
node.addMember( "foo", "Integer" );
node.bindings.append(binding);

FABRIC.close();
