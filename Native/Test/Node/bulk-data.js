/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( "foo", "Integer", 17 );
node.addMember( "bar", "String", "red" );
node.addMember( "baz", "Scalar[]", [3.141] );
console.log(JSON.stringify(node.getBulkData()));
console.log(JSON.stringify(node.getSliceBulkData(0)));
node.setBulkData({foo:[42],bar:["fred"],baz:[[4.5,3.6]]});
console.log(JSON.stringify(node.getBulkData()));
console.log(JSON.stringify(node.getSliceBulkData(0)));
node.resize(2);
console.log(JSON.stringify(node.getBulkData()));
console.log(JSON.stringify(node.getSliceBulkData(0)));
console.log(JSON.stringify(node.getSliceBulkData(1)));
console.log(JSON.stringify(node.getSlicesBulkData([1,0,1])));
node.setSliceBulkData(1, {foo:18,bar:"hello",baz:[]});
console.log(JSON.stringify(node.getBulkData()));
console.log(JSON.stringify(node.getSliceBulkData(0)));
console.log(JSON.stringify(node.getSliceBulkData(1)));
console.log(JSON.stringify(node.getSlicesBulkData([1,0,1])));

FABRIC.close();
