#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( "foo", "Integer[5]" )
node.setData( "foo", [6,4,7,2,1] )
print( node.getDataSize( "foo", 0 ) )
print( node.getDataElement( "foo", 0, 3 ) )

fabricClient.close()
