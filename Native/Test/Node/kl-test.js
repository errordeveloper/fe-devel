/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

parentOp = FABRIC.DependencyGraph.createOperator( "parentOp" );
parentOp.setEntryFunctionName('entry');
parentOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }");

parentBinding = FABRIC.DependencyGraph.createBinding();
parentBinding.setOperator(parentOp);
parentBinding.setParameterLayout([ "self.input", "self.output" ]);

parentNode = FABRIC.DependencyGraph.createNode( "parent" );
parentNode.addMember( "input", "Scalar" );
parentNode.setData( "input", 0, 14 );
parentNode.addMember( "output", "Scalar" );
parentNode.bindings.append(parentBinding);

childOp = FABRIC.DependencyGraph.createOperator( "childOp" );
childOp.setEntryFunctionName('entry');
childOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }");

childBinding = FABRIC.DependencyGraph.createBinding();
childBinding.setOperator(childOp);
childBinding.setParameterLayout([ "parent.output", "self.output" ]);

childNode = FABRIC.DependencyGraph.createNode( "child" );
childNode.setDependency( parentNode, "parent" );
childNode.addMember( "output", "Scalar" );
childNode.bindings.append(childBinding);

childNode.evaluate();
console.log( childNode.getData( 'output', 0 ) );

FABRIC.close();
