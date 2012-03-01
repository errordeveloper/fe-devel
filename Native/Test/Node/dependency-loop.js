/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

node1 = FABRIC.DependencyGraph.createNode("one");
node2 = FABRIC.DependencyGraph.createNode("two");

try {
  node1.setDependency( node1, "reflex" );
  FABRIC.flush();
}
catch (e) {
  console.log( e );
}

try {
  node1.setDependency( node2, "one" );
  node2.setDependency( node1, "two" );
  FABRIC.flush();
}
catch (e) {
  console.log( e );
}

FABRIC.close();
