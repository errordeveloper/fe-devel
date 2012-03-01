/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

cv = FC.MR.createConstValue("Size", 10);

ago = FC.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { output = Scalar(index); }", "foo");
ag = FC.MR.createArrayGenerator(cv, ago);

to = FC.KLC.createArrayTransformOperator("bar.kl", "operator bar(io Scalar value) { value *= 3.14; }", "bar");
t = FC.MR.createArrayTransform(ag, to);

var count = t.getCount();
console.log("t.getCount() = " + count);
for (var i=0; i<count; ++i)
  console.log("t.produce("+i+") = "+t.produce(i));

FC.close();
