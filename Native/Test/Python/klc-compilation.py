#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()
KLC = fabricClient.KLC.createCompilation("foo.kl", "operator foo() { bad }")
print(KLC.getSources())
KLC.addSource("bar.kl", "operator bar() {}")
print(KLC.getSources())
KLE = KLC.run()
print(KLE.getDiagnostics())
KLC.removeSource("foo.kl")
print(KLC.getSources())
KLE = KLC.run()
print(KLE.getDiagnostics())
fabricClient.flush()
fabricClient.close()
