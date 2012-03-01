/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();
n = F.DG.createNode("n");

n.addMember("foo","String[String]");
a = n.getData("foo", 0);
console.log(a);
a.x = "bar";
n.setData("foo",0,a);
a = n.getData("foo", 0);
console.log(a);
a.x = "baz";
n.setData("foo",0,a);
a = n.getData("foo", 0);
console.log(a);

n.addMember("bar","Scalar[Scalar]");
a = n.getData("bar", 0);
console.log(a);
a[2.4] = 3.14;
a[7.9] = 2.71;
n.setData("bar", 0, a );
a = n.getData("bar", 0);
console.log(a);

a = {};
for ( var i=0; i<4096; ++i ) {
  a["key "+i] = "value "+i;
}
n.setData("foo",0,a);
a = n.getData("foo", 0);
console.log(a);

n.addMember("baz","Scalar[String][String]");
a = {
 foo: {
   a: 5.6,
   b: 7.8
 },
 bar: {
   a: 3.1,
   b: -2.1
 }
};
n.setData("baz", 0, a);
a = n.getData("baz", 0);
console.log(a);

F.close();
