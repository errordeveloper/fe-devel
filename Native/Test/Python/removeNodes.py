#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()


dgnode1 = fabricClient.DependencyGraph.createNode("node1")
dgnode2 = fabricClient.DependencyGraph.createNode("node2")
dgnode2.setDependency(dgnode1, 'node1')
print( "Node 2 dep: " + str(dgnode2.getDependencies()) )

#x = input('Test')


fabricClient.close()

