/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

op = FABRIC.DependencyGraph.createOperator( "op" );
op.setEntryFunctionName('entry');
op.setSourceCode("operator entry( io Scalar input ) { String foo; report foo; }");

binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( [ "self.input" ] );

node = FABRIC.DependencyGraph.createNode( "node" );
node.addMember( "input", "Scalar" );
node.bindings.append( binding );
node.setData( "input", 17 );
node.evaluate();

FABRIC.close();
