/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var event = FABRIC.DependencyGraph.createEvent("event");
var eventHandler = FABRIC.DependencyGraph.createEventHandler("eventHandler");
event.appendEventHandler( eventHandler );
var node = FABRIC.DependencyGraph.createNode( "node" );
eventHandler.setScope( 'self', node );

var nodeOne = FABRIC.DependencyGraph.createNode( "nodeOne" );
var operatorOne = FABRIC.DependencyGraph.createOperator( "operatorOne" );
operatorOne.setEntryFunctionName('test');
operatorOne.setSourceCode("operator test( io Boolean select, io Scalar value ) { select = true; value = 7; }");
var bindingOne = FABRIC.DependencyGraph.createBinding();
bindingOne.setOperator( operatorOne );
bindingOne.setParameterLayout( [] );
var eventHandlerOne = FABRIC.DependencyGraph.createEventHandler("eventHandlerOne");
eventHandlerOne.setScope( 'self', nodeOne );
eventHandlerOne.setSelector( 'self', bindingOne );
eventHandler.appendChildEventHandler( eventHandlerOne );

var nodeTwo = FABRIC.DependencyGraph.createNode( "nodeTwo" );
var operatorTwo = FABRIC.DependencyGraph.createOperator( "operatorTwo" );
operatorTwo.setEntryFunctionName('test');
operatorTwo.setSourceCode("operator test( io Boolean select, io Scalar value ) { value = 4; }");
var bindingTwo = FABRIC.DependencyGraph.createBinding();
bindingTwo.setOperator( operatorTwo );
bindingTwo.setParameterLayout( [] );
var eventHandlerTwo = FABRIC.DependencyGraph.createEventHandler("eventHandlerTwo");
eventHandlerTwo.setScope( 'self', nodeTwo );
eventHandlerTwo.setSelector( 'self', bindingTwo );
eventHandler.appendChildEventHandler( eventHandlerTwo );
         
event.setSelectType( 'Scalar' );
var result = event.select();

if ( result.length != 1 ) {
  console.log( "incorrect result length" );
}

if ( result[0].value != 7 ) {
  console.log( "incorrect result value" );
}

FABRIC.close();
