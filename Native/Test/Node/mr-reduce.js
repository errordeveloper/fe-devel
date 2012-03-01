/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();

cv = FC.MR.createConstValue("Size", 10000000);

ago = FC.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Float64 output, Size index) { output = Float64(index+1); }", "foo");
ag = FC.MR.createArrayGenerator(cv, ago);

mo = FC.KLC.createArrayMapOperator("bar.kl", "operator bar(Float64 input, io Float64 output) { output = 1.0 / (input * input); }", "bar");
m = FC.MR.createArrayMap(ag, mo);

ro = FC.KLC.createReduceOperator("reduce.kl", "operator reduce(Float64 input, io Float64 result) { result += input; }", "reduce");
r = FC.MR.createReduce(m, ro);

console.log(r.produce());

FC.close();
