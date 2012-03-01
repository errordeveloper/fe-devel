#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

parentNode = F.DG.createNode('parentNode')
childNode = F.DG.createNode('childNode')
childNode.setDependency(parentNode, 'parent')

def printDeps( dependencies ):
  result = []
  for dependency in dependencies:
    result.append( dependency )
  print result

printDeps( childNode.getDependencies() )

try:
  childNode.removeDependency('notAParent')
  F.flush()
except Exception as e:
  print("childNode.removeDependency('notAParent'): " + str(e))

childNode.removeDependency('parent')
print(childNode.getDependencies())

F.close()
