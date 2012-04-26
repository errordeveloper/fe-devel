/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

opOne = FABRIC.DependencyGraph.createOperator( "opOne" );
opOne.setEntryPoint('entry');
opOne.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }");

bindingOne = FABRIC.DependencyGraph.createBinding();
bindingOne.setOperator( opOne );
bindingOne.setParameterLayout( [ "self.input", "self.output" ] );

opTwo = FABRIC.DependencyGraph.createOperator( "opTwo" );
opTwo.setEntryPoint('entry');
opTwo.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 3 * input; }");

bindingTwo = FABRIC.DependencyGraph.createBinding();
bindingTwo.setOperator( opTwo );
bindingTwo.setParameterLayout( [ "self.input", "self.output" ] );

node = FABRIC.DependencyGraph.createNode( "parent" );
node.addMember( "input", "Scalar" );
node.addMember( "output", "Scalar" );
node.resize( 2 );
node.setData( 'input', 0, 3 );
node.setData( 'input', 1, 7 );
node.setData( 'output', 0, 0 );
node.setData( 'output', 1, 0 );
node.evaluate();
console.log(JSON.stringify(node.getBulkData()));
console.log("bindings.length = " + node.bindings.getLength());

node.setData( 'output', 0, 0 );
node.setData( 'output', 1, 0 );
node.bindings.append( bindingOne );
node.evaluate();
console.log(JSON.stringify(node.getBulkData()));
console.log("bindings.length = " + node.bindings.getLength());

node.setData( 'output', 0, 0 );
node.setData( 'output', 1, 0 );
node.bindings.remove(0);
node.evaluate();
console.log(JSON.stringify(node.getBulkData()));
console.log("bindings.length = " + node.bindings.getLength());

node.setData( 'output', 0, 0 );
node.setData( 'output', 1, 0 );
node.bindings.append(bindingTwo);
node.evaluate();
console.log(JSON.stringify(node.getBulkData()));
console.log("bindings.length = " + node.bindings.getLength());

node.setData( 'output', 0, 0 );
node.setData( 'output', 1, 0 );
node.bindings.remove(0);
node.evaluate();
console.log(JSON.stringify(node.getBulkData()));
console.log("bindings.length = " + node.bindings.getLength());

FABRIC.close();
