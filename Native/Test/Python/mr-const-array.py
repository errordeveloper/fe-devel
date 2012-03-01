#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

ca = client.MR.createConstArray("String", ["one","two","three","four","five"])

count = ca.getCount()
print("ca.getCount() = " + str(count))
for i in range(0, count):
  print("ca.produce("+str(i)+") = " + str(ca.produce(i)))

client.close()
