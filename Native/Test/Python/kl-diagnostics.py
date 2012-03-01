#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

op = fabricClient.DependencyGraph.createOperator( "op" )
print( len( op.getDiagnostics() ) )

op.setSourceCode("operator entry( io Scalar input, io Scalar output ) { foo; }")
print( len( op.getDiagnostics() ) )
print( op.getDiagnostics()[0]['desc'] )

op.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }")
print( len( op.getDiagnostics() ) )

fabricClient.close()
