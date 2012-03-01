#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

# taken from the register-types test, just the small
# segment that causes the memory leak
class ComplexType():
  def __init__( self ):
    mystr = "foooooooo"
    vec = []
    for i in range(0, 4):
      vec.append( mystr )
    vec_vec = []
    for i in range(0, 6):
      vec_vec.append( vec )
    self.member = vec_vec

complexTypeDesc = {
  'members': [ { 'member': 'String[][]' } ],
  'constructor': ComplexType
}

fabricClient.RT.registerType('ComplexType', complexTypeDesc)
fabricClient.flush()
fabricClient.close()

