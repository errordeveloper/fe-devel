#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue("Size", 10)

ago = client.KLC.createArrayGeneratorOperator("foo.kl", "operator foo(io Scalar output, Size index) { report 'Running generator'; output = sqrt(Scalar(index)); }", "foo");
ag = client.MR.createArrayGenerator(cv, ago)

cache = client.MR.createArrayCache(ag)

count = ag.getCount()
print("cache.getCount() = " + str(count))

print("Should run generator")
for i in range(0, count):
  print("cache.produce("+str(i)+") = "+str(cache.produce(i)))

print("Should not run generator (cached)")
for i in range(0, count):
  print("cache.produce("+str(i)+") = "+str(cache.produce(i)))

cache.flush()
print("Should run generator")
for i in range(0, count):
  print("cache.produce("+str(i)+") = "+str(cache.produce(i)))

client.close()
