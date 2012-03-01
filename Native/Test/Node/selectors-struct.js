/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var Vec3 = function( x, y, z ) {
  if ( typeof x === "number" && typeof y === "number" && typeof z === "number" ) {
    this.x = x;
    this.y = y;
    this.z = z;
  }
  else if ( x === undefined && y === undefined && z === undefined ) {
    this.x = 0;
    this.y = 0;
    this.z = 0;
  }
  else throw "new Vec3: invalid arguments";
};

var desc = {
  members: { x:'Scalar', y:'Scalar', z:'Scalar' },
  constructor: Vec3,
  kBindings: "\
function Vec3( Scalar x, Scalar y, Scalar z )\n\
{\n\
  this.x = x;\n\
  this.y = y;\n\
  this.z = z;\n\
}\n\
"
};

FABRIC.RegisteredTypesManager.registerType( 'Vec3', desc );

var event, eventHandler, node;
event = FABRIC.DependencyGraph.createEvent("event");
eventHandler = FABRIC.DependencyGraph.createEventHandler("eventHandler");
event.appendEventHandler( eventHandler );
node = FABRIC.DependencyGraph.createNode( "node" );
eventHandler.setScope( 'self', node );

var nodeOne, operatorOne, bindingOne, eventHandlerOne;
nodeOne = FABRIC.DependencyGraph.createNode( "nodeOne" );

operatorOne = FABRIC.DependencyGraph.createOperator( "operatorOne" );
operatorOne.setEntryFunctionName('test');
operatorOne.setSourceCode("operator test( io Boolean select, io Vec3 value ) { select = true; value.x = 7; }");

bindingOne = FABRIC.DependencyGraph.createBinding();
bindingOne.setOperator( operatorOne );
bindingOne.setParameterLayout( [] );

eventHandlerOne = FABRIC.DependencyGraph.createEventHandler("eventHandlerOne");
eventHandlerOne.setScope( 'self', nodeOne );
eventHandlerOne.setSelector( 'self', bindingOne );
eventHandler.appendChildEventHandler( eventHandlerOne );

var nodeTwo = FABRIC.DependencyGraph.createNode( "nodeTwo" );
var operatorTwo = FABRIC.DependencyGraph.createOperator( "operatorTwo" );
operatorTwo.setEntryFunctionName('test');
operatorTwo.setSourceCode("operator test( io Boolean select, io Vec3 value ) { value.x = 4; }");
var bindingTwo = FABRIC.DependencyGraph.createBinding();
bindingTwo.setOperator( operatorTwo );
bindingTwo.setParameterLayout( [] );
var eventHandlerTwo = FABRIC.DependencyGraph.createEventHandler("eventHandlerTwo");
eventHandlerTwo.setScope( 'self', nodeTwo );
eventHandlerTwo.setSelector( 'self', bindingTwo );
eventHandler.appendChildEventHandler( eventHandlerTwo );

event.setSelectType( 'Vec3' );
var result = event.select();

if ( result.length != 1 ) {
  console.log( "incorrect number of results" );
}

if ( result[0].value.x != 7 ) {
  console.log( "incorrect value of results" );
}

FABRIC.close();
