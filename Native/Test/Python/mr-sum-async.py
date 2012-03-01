#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 100)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Float64 output, Size index) { output = Float64(index+1); }", "foo")
ag = client.MR.createArrayGenerator(cv, ago)

mo = client.KLC.createArrayMapOperator("map.kl", "operator map(Float64 input, io Float64 output, Size index, Size count, Scalar multiplier) { output = input * multiplier; }", "map")
sv = client.MR.createConstValue("Scalar", 3.14)
m = client.MR.createArrayMap(ag, mo, sv)

ro = client.KLC.createReduceOperator("reduce.kl", "operator reduce(Float64 input, io Float64 result, Size index, Size count, Scalar multiplier) { result += input * multiplier; }", "reduce")
sv = client.MR.createConstValue("Scalar", 3.14)
r = client.MR.createReduce(m, ro, sv)


def callback( result ):
  print( result )
  client.close()

r.produceAsync( callback )

