/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

op = FABRIC.DependencyGraph.createOperator( "op" );
console.log( op.getDiagnostics().length );

op.setSourceCode("operator entry( io Scalar input, io Scalar output ) { foo; }");
console.log( op.getDiagnostics().length );
console.log( op.getDiagnostics()[0].desc );

op.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }");
console.log( op.getDiagnostics().length );

FABRIC.close();
