#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

n = F.DG.createNode('n')
n.addMember('foo', 'String')
n.resize(3)
for i in range( 0, 3 ):
  n.setData('foo', i, "bar-"+str(i))

print(n.getMemberBulkData('foo'))
try:
  print(n.getMemberBulkData('bar'))
except Exception as e:
  print('Caught exception: ' + str(e))

print(n.getMembersBulkData(['foo']))
try:
  print(n.getMembersBulkData(['foo','bar']))
except Exception as e:
  print('Caught exception: ' + str(e))

F.close()
