/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

cv = FC.MR.createConstValue("Size", 100);

ago = FC.KLC.createArrayGeneratorOperator("foo.kl", "require FabricMath; operator foo(io Size output, Size index) { output = Size(round(1000.0*mathRandomFloat32(0, index))); }", "foo");
ag = FC.MR.createArrayGenerator(cv, ago);

var count = ag.getCount();
console.log("ag.getCount() = " + count);
for (var i=0; i<count; ++i)
  console.log("ag.produce("+i+") = "+ag.produce(i));

FC.close();
