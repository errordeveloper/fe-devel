#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabrictest
import fabric
F = fabric.createClient()

eh = F.DG.createEventHandler("event handler")
print(eh.getName())
print(eh.getType())

def mapNamedObjectsToNames( objects ):
  result = []
  for i in range( 0, len( objects ) ):
    result.append( objects[i].getName() )
  return result

print(fabrictest.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))))

childEventHandler = F.DG.createEventHandler("child event handler")
eh.appendChildEventHandler(childEventHandler)
print(fabrictest.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))))

eh.appendChildEventHandler(F.DG.createEventHandler("another child event handler"))
print(fabrictest.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))))

eh.removeChildEventHandler(childEventHandler)
print(fabrictest.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))))

F.close()
