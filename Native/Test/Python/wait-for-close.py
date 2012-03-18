#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
import time

fabricClient = fabric.createClient()

ap = fabricClient.MR.createArrayGenerator(
  fabricClient.MR.createConstValue('Size', 16),
  fabricClient.KLC.createArrayGeneratorOperator(
    'foo.kl',
    'operator foo(io Scalar x, Size index) { x = 3.141 * x * index; }',
    'foo'
    )
  )

def callback1( result ):
  # this may get called too fast unless we pause here
  time.sleep(1)
  print 'closing...'
  fabricClient.close()

ap.produceAsync( callback1 )

print 'waiting for close...'
fabricClient.waitForClose()
print 'closed'

