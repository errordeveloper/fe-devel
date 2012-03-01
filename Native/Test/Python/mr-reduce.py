#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 10000000)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Float64 output, Size index) { output = Float64(index+1); }", "foo")
ag = client.MR.createArrayGenerator(cv, ago)

mo = client.KLC.createArrayMapOperator("bar.kl", "operator bar(Float64 input, io Float64 output) { output = 1.0 / (input * input); }", "bar")
m = client.MR.createArrayMap(ag, mo)

ro = client.KLC.createReduceOperator("reduce.kl", "operator reduce(Float64 input, io Float64 result) { result += input; }", "reduce")
r = client.MR.createReduce(m, ro)

print(r.produce())

client.close()
