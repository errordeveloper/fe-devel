/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "foo", "Integer[]" );
node.setData( "foo", 0, [1,6,7,9,8] );
console.log( node.getDataSize( "foo", 0 ) );
console.log( node.getDataElement( "foo", 0, 3 ) );

FABRIC.close();
