/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();
            
op = FABRIC.DependencyGraph.createOperator("bar");
op.setEntryFunctionName("foo");
op.setSourceCode("operator foo( Boolean testBool, io Integer foo ) { if(testBool){ foo = 10; } }");

binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( [ "self.testBool", "self.foo" ] );

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "testBool", "Boolean" );
node.addMember( "foo", "Integer" );
node.bindings.append(binding);

var errors = node.getErrors();
console.log( errors.length );
console.log( errors[0] );

FABRIC.close();
