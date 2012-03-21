/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

var getSortedKeysString = function( dict ) {
  var keys = [];
  for (var key in dict) {
    if (dict.hasOwnProperty(key))
      keys.push(key);
  }
  keys.sort();
  return JSON.stringify( keys );
}

fabricClient = require('Fabric').createClient();

dgnode1 = fabricClient.DependencyGraph.createNode("node1");
dgnode2 = fabricClient.DependencyGraph.createNode("node2");
dgnode3 = fabricClient.DependencyGraph.createNode("node3");
dgnode2.setDependency(dgnode1, 'node1');
dgnode3.setDependency(dgnode2, 'node2');

evhan1 = fabricClient.DependencyGraph.createEventHandler("evhan1");
evhan2 = fabricClient.DependencyGraph.createEventHandler("evhan2");
evhan3 = fabricClient.DependencyGraph.createEventHandler("evhan3");
evhan2.setScope("node2", dgnode2);
evhan1.appendChildEventHandler( evhan2 );
evhan2.appendChildEventHandler( evhan3 );

event1 = fabricClient.DependencyGraph.createEvent( "event1" );
event2 = fabricClient.DependencyGraph.createEvent( "event2" );
event1.appendEventHandler( evhan1 );
event2.appendEventHandler( evhan2 );

fabricClient.flush()

for ( var dep in dgnode3.getDependencies() )
  console.log( "Node3 initial dependency: " + dep )

for ( var index in evhan1.getChildEventHandlers() )
  console.log( "EvHan1 initial child event handler: " + evhan1.getChildEventHandlers()[index].getName() )

for ( var dep in evhan2.getScopes() )
  console.log( "EvHan2 initial dependency: " + dep )

for ( var index in event2.getEventHandlers() )
  console.log( "Event2 initial event handlers: " + event2.getEventHandlers()[index].getName() )

console.log( "Initial named objects: " + getSortedKeysString( fabricClient.DependencyGraph.getAllNamedObjects() ) );
console.log( "Is Node2 valid: " + dgnode2.isValid() );

dgnode2.destroy()

try
{
  dgnode2.setDependency(dgnode1, 'node1Again');
}
catch(e)
{
  console.log('Error on modifying a destroyed Node: ' + e);
}

fabricClient.flush()

console.log( "Is Node2 valid: " + dgnode2.isValid() );
console.log( "Named objects after deleted Node2: " + getSortedKeysString( fabricClient.DependencyGraph.getAllNamedObjects() ) );

for ( var dep in dgnode3.getDependencies() )
  console.log( "ERROR, dependency not cleaned up" );

for ( var dep in evhan2.getScopes() )
  console.log( "ERROR, scope not cleaned up" );

for ( var index in evhan1.getChildEventHandlers() )
  console.log( "EvHan1 child event handler: " + evhan1.getChildEventHandlers()[index].getName() );

evhan2.destroy();
fabricClient.flush();

for ( var dep in evhan1.getChildEventHandlers() )
  console.log( "ERROR, child event handlers not cleaned up" )

for ( var dep in event2.getEventHandlers() )
  console.log( "ERROR, event handlers not cleaned up" )

console.log( "Named objects after deleted EvHan2: " + getSortedKeysString( fabricClient.DependencyGraph.getAllNamedObjects() ) );

event1.destroy();
fabricClient.flush();

console.log( "Named objects after deleted Ev1: " + getSortedKeysString( fabricClient.DependencyGraph.getAllNamedObjects() ) );

fabricClient.close();
