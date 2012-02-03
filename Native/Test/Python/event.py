import fabric
F = fabric.createClient()

def mapNamedObjectsToNames( objects ):
  result = []
  for i in range( 0, len( objects ) ):
    result.append( objects[i].getName() )
  return result

e = F.DG.createEvent("event")
print(e.getName())
print(e.getType())

print(fabric.stringify(mapNamedObjectsToNames(e.getEventHandlers())))
eh = F.DG.createEventHandler("eventHandler")
e.appendEventHandler(eh)
print(fabric.stringify(mapNamedObjectsToNames(e.getEventHandlers())))

print(e.getDidFireCallback())

def fireCallback( self ):
  print( self.getName() + " fired" )

e.setDidFireCallback( fireCallback )

print(type( e.getDidFireCallback() ))
e.fire()

F.close()
