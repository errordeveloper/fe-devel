import fabric
import json

def stringify( obj ):
  return json.dumps( _normalize( fabric._typeToDict( obj ) ) )

# for unit tests only, make floats use same precision across different
# versions of python which have different repr() implementations and
# change dicts to sorted lists so ordering doesn't change
def _normalize( obj ):
  if type( obj ) is list:
    objlist = []
    for elem in obj:
      objlist.append( _normalize( elem ) )
    return objlist
  elif type( obj ) is dict:
    objdictlist = []
    for member in obj:
      elemobj = {}
      elemobj[ member ] = _normalize( obj[ member ] )
      objdictlist.append( elemobj )
    objdictlist.sort()
    return objdictlist
  elif type( obj ) is float:
    return format( obj, '.3f' )
  else:
    return obj

