/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

ca = FC.MR.createConstArray("String", ["one","two","three","four","five"]);

var count = ca.getCount();
console.log("ca.getCount() = " + count);
for (var i=0; i<count; ++i)
  console.log("ca.produce("+i+") = " + ca.produce(i));

FC.close();
