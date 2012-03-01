/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

n = F.DG.createNode('n');
n.addMember('foo', 'String');
n.resize(3);
for (var i=0; i<3; ++i) {
  n.setData('foo', i, "bar-"+i);
}

console.log(n.getMemberBulkData('foo'));
try {
  console.log(n.getMemberBulkData('bar'));
}
catch (e) {
  console.log('Caught exception: ' + e);
}

console.log(n.getMembersBulkData(['foo']));
try {
  console.log(n.getMembersBulkData(['foo','bar']));
}
catch (e) {
  console.log('Caught exception: ' + e);
}

F.close();
