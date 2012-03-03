#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

try:
  fabricClient.RT.registerType( 'Foobar1', 'foobar' )
  fabricClient.flush()
except Exception as e:
  print 'error registering Foobar1: ' + str( e )

try:
  desc = {
    'members': { 'x' },
    'constructor': object
  }
  fabricClient.RT.registerType( 'Foobar2', desc )
  fabricClient.flush()
except Exception as e:
  print 'error registering Foobar2: ' + str( e )

try:
  desc = {
    'constructor': object
  }
  fabricClient.RT.registerType( 'Foobar3', desc )
  fabricClient.flush()
except Exception as e:
  print 'error registering Foobar3: ' + str( e )

desc = {
  'members': [ { 'x':'Scalar' }, { 'y':'Scalar' } ]
}
fabricClient.RT.registerType( 'Foobar4', desc )
node = fabricClient.DG.createNode( 'node1' )
node.addMember( 'myFoo3', 'Foobar4' )
data = node.getData( 'myFoo3', 0 )
print 'Foobar4: have x = ' + str( data.x ) + ', y = ' + str( data.y )

try:
  print data.z
except Exception as e:
  print( 'no z element exists' )

fabricClient.close()

