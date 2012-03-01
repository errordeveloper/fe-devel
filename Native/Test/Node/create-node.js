/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "foo", "Integer", 17 );
console.log( node.getData( "foo" ) );
node.setData( "foo", 42 );
console.log( node.getData( "foo" ) );

FABRIC.close();
