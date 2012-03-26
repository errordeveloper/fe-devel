#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

desc = {
  'members': [ { 'x':'Scalar' }, { 'y':'Data' } ]
}

fabricClient.RegisteredTypesManager.registerType( 'DataMember', desc )
print(fabric.stringify(fabricClient.RT.getRegisteredTypes()['DataMember']))

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( 'foo', 'DataMember' )
data = node.getData("foo", 0)
print( fabric.stringify( data ) )

data.x = 1.0
data.y = True
node.setData("foo", 0, data)

data = node.getData("foo", 0)
print( fabric.stringify( data ) )

fabricClient.close()

