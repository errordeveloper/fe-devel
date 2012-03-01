/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

cv = FC.MR.createConstValue("Size", 10);

ago = FC.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { output = sqrt(Scalar(index)); }", "foo");
ag = FC.MR.createArrayGenerator(cv, ago);

var count = ag.getCount();
console.log("ag.getCount() = " + count);
for (var i=0; i<count; ++i)
  console.log("ag.produce("+i+") = "+ag.produce(i));

FC.close();
