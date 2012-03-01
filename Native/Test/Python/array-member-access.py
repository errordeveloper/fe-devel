#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( "foo", "Integer[]" )
node.setData( "foo", 0, [1,6,7,9,8] )
print( node.getDataSize( "foo", 0 ) )
print( node.getDataElement( "foo", 0, 3 ) )

fabricClient.close()
