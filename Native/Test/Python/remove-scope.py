#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

node = F.DG.createNode('node')
eventHandler = F.DG.createEventHandler('eventHandler')
eventHandler.setScope('node', node)

def printScopes( scopes ):
  result = []
  for scope in scopes:
    result.append( scope )
  print result

printScopes( eventHandler.getScopes() )

try:
  eventHandler.removeScope('notANode')
  F.flush()
except Exception as e:
  print("eventHandler.removeScope('notANode'): " + str(e))

eventHandler.removeScope('node')
print(eventHandler.getScopes())

F.close()
