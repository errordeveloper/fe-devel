/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var reportErrors = function( obj, expected ) {
  var numErrors = obj.getErrors().length;
  if ( numErrors != expected ) {
    console.log( obj.getName() + " errors = " + numErrors +
      ", expected " + expected );
  }
};

var event;
event = FABRIC.DependencyGraph.createEvent("bar");
event.fire();

reportErrors( event, 0 );

var eventHandler;
eventHandler = FABRIC.DependencyGraph.createEventHandler("baz");
event.appendEventHandler( eventHandler );
event.fire();

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );

var node;
node = FABRIC.DependencyGraph.createNode("foo");
eventHandler.setScope( "self", node );
event.fire();

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );
reportErrors( node, 0 );

var op, binding;
op = FABRIC.DependencyGraph.createOperator("fooTwo");
binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( [ "self.foo" ] );
node.bindings.append( binding );

try {
  event.fire();
}
catch (e) {
  console.log( "create and push operator: fire event successfully fails" );
}

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );
reportErrors( node, 3 );
 
op.setEntryFunctionName("foo");
op.setSourceCode("operator foo( io Integer bar ) { bar = 7; }");

try {
  event.fire();
}
catch (e) {
  console.log( "load operator bytecode: fire event successfully fails" );
}

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );
reportErrors( node, 1 );

node.addMember( "foo", "Integer" );
event.fire();

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );
reportErrors( node, 0 );

if ( node.getData('foo') != 7 ) {
  console.log( "node.getData('foo') != 7" );
}

node.removeMember( "foo" );
try {
  event.fire();
}
catch (e) {
  console.log( "remove node member: fire event successfully fails" );
}

reportErrors( event, 0 );
reportErrors( eventHandler, 0 );
reportErrors( node, 1 );

FABRIC.close();
