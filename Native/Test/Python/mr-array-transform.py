#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 10)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { output = Scalar(index); }", "foo")
ag = client.MR.createArrayGenerator(cv, ago)

to = client.KLC.createArrayTransformOperator("bar.kl", "operator bar(io Scalar value) { value *= 3.14; }", "bar")
t = client.MR.createArrayTransform(ag, to)

count = t.getCount()
print("t.getCount() = " + str(count))
for i in range(0, count):
  print("t.produce("+str(i)+") = "+str(round(t.produce(i),3)))

client.close()
