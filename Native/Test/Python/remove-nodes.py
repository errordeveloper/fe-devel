#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

dgnode1 = fabricClient.DependencyGraph.createNode("node1")
dgnode2 = fabricClient.DependencyGraph.createNode("node2")
dgnode3 = fabricClient.DependencyGraph.createNode("node3")
dgnode2.setDependency(dgnode1, 'node1')
dgnode3.setDependency(dgnode2, 'node2')

evhan1 = fabricClient.DependencyGraph.createEventHandler("evhan1")
evhan2 = fabricClient.DependencyGraph.createEventHandler("evhan2")
evhan3 = fabricClient.DependencyGraph.createEventHandler("evhan3")
evhan2.setScope("node2", dgnode2)
evhan1.appendChildEventHandler( evhan2 )
evhan2.appendChildEventHandler( evhan3 )

event1 = fabricClient.DependencyGraph.createEvent( "event1" )
event2 = fabricClient.DependencyGraph.createEvent( "event2" )
event1.appendEventHandler( evhan1 )
event2.appendEventHandler( evhan2 )

fabricClient.flush()

for dep in dgnode3.getDependencies():
  print( "Node3 initial dependency: " + dep )

for dep in evhan1.getChildEventHandlers():
  print( "EvHan1 initial child event handler: " + dep.getName() )

for dep in evhan2.getScopes():
  print( "EvHan2 initial dependency: " + dep )

for dep in event2.getEventHandlers():
  print( "Event2 initial event handlers: " + dep.getName() )

print( "Initial named objects: " + str( sorted(fabricClient.DependencyGraph.getAllNamedObjects().keys()) ) );
print( "Is Node2 valid: " + str(dgnode2.isValid()) );

dgnode2.destroy()

try:
  dgnode2.setDependency(dgnode1, 'node1Again')
except Exception as e:
  print('Error on modifying a destroyed Node: ' + str(e))

fabricClient.flush()

print( "Is Node2 valid: " + str(dgnode2.isValid()) );
print( "Named objects after deleted Node2: " + str( sorted(fabricClient.DependencyGraph.getAllNamedObjects().keys()) ) );

for dep in dgnode3.getDependencies():
 print( "ERROR, dependency not cleaned up" )

for dep in evhan2.getScopes():
  print( "ERROR, scope not cleaned up" )

for dep in evhan1.getChildEventHandlers():
  print( "EvHan1 child event handler: " + dep.getName() )

evhan2.destroy()
fabricClient.flush()

for dep in evhan1.getChildEventHandlers():
  print( "ERROR, child event handlers not cleaned up" )

for dep in event2.getEventHandlers():
  print( "ERROR, event handlers not cleaned up" )

print( "Named objects after deleted EvHan2: " + str( sorted(fabricClient.DependencyGraph.getAllNamedObjects().keys()) ) );

event1.destroy()
fabricClient.flush()

print( "Named objects after deleted Ev1: " + str( sorted(fabricClient.DependencyGraph.getAllNamedObjects().keys()) ) );

fabricClient.close()

