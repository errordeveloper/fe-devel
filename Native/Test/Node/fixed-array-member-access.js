/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "foo", "Integer[5]" );
node.setData( "foo", [6,4,7,2,1] );
console.log( node.getDataSize( "foo", 0 ) );
console.log( node.getDataElement( "foo", 0, 3 ) );

FABRIC.close();
