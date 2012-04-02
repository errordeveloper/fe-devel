/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var node = FABRIC.DG.createNode("node");
node.addMember("foo","Scalar",3.141);
console.log(Math.round(node.getData("foo", 0)*10000)/10000);
node.setData("foo", 0, 2.718);
console.log(Math.round(node.getData("foo", 0)*10000)/10000);
node.resize(2);
console.log(Math.round(node.getData("foo", 1)*10000)/10000);

FABRIC.close();
