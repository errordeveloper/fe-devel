FABRIC = require('Fabric').createClient();

node = FABRIC.DependencyGraph.createNode( "node" );
node.addMember( "input", "Scalar" );
node.setData( "input", 17 );

op = FABRIC.DependencyGraph.createOperator( "op" );
op.setEntryFunctionName('entry');
op.setSourceCode("operator entry( io Scalar input ) {}");

binding = FABRIC.DependencyGraph.createBinding();
binding.setOperator( op );
binding.setParameterLayout( [ "node.doesntExist" ] );

eh1 = FABRIC.DependencyGraph.createEventHandler("eventHandlerOne");
eh1.preDescendBindings.append(binding);
eh1.setScope( 'node', node );

eh2 = FABRIC.DependencyGraph.createEventHandler("eventHandlerTwo");

e = FABRIC.DependencyGraph.createEvent("event");
e.appendEventHandler( eh1 );
e.appendEventHandler( eh2 );

console.log( "Errors on eh1:" );
console.log( eh1.getErrors() );
console.log( "Errors on eh2:" );
console.log( eh2.getErrors() );

FABRIC.flush();
