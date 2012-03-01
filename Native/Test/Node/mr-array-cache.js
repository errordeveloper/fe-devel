/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

cv = FC.MR.createConstValue("Size", 10);

ago = FC.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { report 'Running generator'; output = sqrt(Scalar(index)); }", "foo");
ag = FC.MR.createArrayGenerator(cv, ago);

var cache = FC.MR.createArrayCache(ag);

var count = ag.getCount();
console.log("cache.getCount() = " + count);

console.log("Should run generator");
for (var i=0; i<count; ++i)
  console.log("cache.produce("+i+") = "+cache.produce(i));

console.log("Should not run generator (cached)");
for (var i=0; i<count; ++i)
  console.log("cache.produce("+i+") = "+cache.produce(i));

cache.flush();
console.log("Should run generator");
for (var i=0; i<count; ++i)
  console.log("cache.produce("+i+") = "+cache.produce(i));

FC.close();
