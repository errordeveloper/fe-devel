#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
F = fabric.createClient()

o = F.DG.createOperator("op")
o.setEntryFunctionName("entry")
o.setSourceCode("\
operator entry( io String string )\n\
{\n\
  string = 'foobar';\n\
}\n\
")

binding = F.DependencyGraph.createBinding()
binding.setOperator( o )
binding.setParameterLayout( [ "self.string" ] )

eh = F.DG.createEventHandler("eh")
eh.addMember( "string", "String" )
eh.preDescendBindings.append( binding )

e = F.DG.createEvent("e")
e.appendEventHandler( eh )
e.fire()
print( eh.getData( "string" ) )

F.close()
