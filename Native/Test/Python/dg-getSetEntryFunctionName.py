#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

o = F.DG.createOperator("op")
o.setSourceCode("operator entry() { report 'Hello'; }")
o.setEntryFunctionName("entry")
print(o.getSourceCode())
print(o.getEntryFunctionName())

F.close()
