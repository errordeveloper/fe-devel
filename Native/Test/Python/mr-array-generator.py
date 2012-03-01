#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 10)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { output = sqrt(Scalar(index)); }", "foo")
ag = client.MR.createArrayGenerator(cv, ago)

count = ag.getCount()
print("ag.getCount() = " + str(count))
for i in range(0, count):
  print("ag.produce("+str(i)+") = "+str(ag.produce(i)))

client.close()
