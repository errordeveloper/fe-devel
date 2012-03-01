#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

myvars = {}
myvars['completed'] = 0

def launchTest( fabricClient ):
  o = fabricClient.DG.createOperator("o"+str(testIndex))
  o.setSourceCode("inline", "operator entry(in Size index, io Scalar result) { for (Size i=0; i<4096; ++i) result = log(1+index+i); }")
  o.setEntryFunctionName("entry")

  b = fabricClient.DG.createBinding();
  b.setOperator(o)
  b.setParameterLayout(["self.index","self.a"])

  n = fabricClient.DG.createNode("n"+str(testIndex))
  n.addMember("a", "Scalar")
  n.resize(10000)
  n.bindings.append(b)

  def complete():
    myvars['completed'] = myvars['completed'] + 1
    print "completed " + str(myvars['completed'])
    fabricClient.close()

  n.evaluateAsync( complete )

for testIndex in range( 0, 64 ):
  launchTest( fabric.createClient() )
  print("queued "+str(testIndex))

