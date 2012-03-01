#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 10)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { output = Scalar(index); }", "foo")
ag = client.MR.createArrayGenerator(cv, ago)

mo = client.KLC.createArrayMapOperator("bar.kl", "operator bar(Scalar input, io Scalar output) { output = input * input; }", "bar")
m = client.MR.createArrayMap(ag, mo)

count = m.getCount()
print("count = " + str(count))
for i in range(0, count):
  print("produce("+str(i)+") = "+str(m.produce(i)))

client.close()
