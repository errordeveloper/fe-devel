#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

desc = {
  'members': [ { 'x':'Scalar' }, { 'y':'Scalar' } ]
}

fabricClient.RT.registerType( 'Foobar', desc )
node = fabricClient.DG.createNode( 'node' )
node.addMember( 'myFoo', 'Foobar' )
data = node.getData( 'myFoo', 0 )

fabricClient.close()

