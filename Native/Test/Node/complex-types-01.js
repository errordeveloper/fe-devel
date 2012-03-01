/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var MyStruct = function( i, s ) {
  if ( typeof i === "number" && typeof s === "number" ) {
    this.i = i;
    this.s = s;
  }
  else if ( i === undefined && s === undefined ) {
    this.i = 0;
    this.s = 0.0;
  }
  else throw "invalid use of constructor";
};
myStructDesc = {
  members: {
    i: "Integer",
    s: "Scalar"
  },
  constructor: MyStruct
};
FABRIC.RegisteredTypesManager.registerType( "MyStruct", myStructDesc );

op = FABRIC.DependencyGraph.createOperator( "op" );
op.setEntryFunctionName( "entry" );
op.setSourceCode( "\n\
operator entry( io MyStruct arg[] )\n\
{\n\
  var MyStruct ms;\n\
  ms.i = 42;\n\
  ms.s = 3.141;\n\
  \n\
  arg.push( ms );\n\
}\n\
" );
var diagnostics = op.getDiagnostics();
if ( diagnostics.length > 0 ) {
  for ( var i in diagnostics ) {
    console.log( diagnostics[i].line + ": " + diagnostics[i].desc );
  }
  console.log( "Full code:" );
  console.log( op.getFullSourceCode() );
}
else {
	var binding = FABRIC.DG.createBinding();
	binding.setOperator(op);
	binding.setParameterLayout(["self.msaa"]);
	
  node = FABRIC.DependencyGraph.createNode( "node" );
  node.addMember( "msaa", "MyStruct[]" );
  node.bindings.append(binding);
	var errors = node.getErrors();
  if ( errors.length > 0 ) {
    for ( var i in errors ) {
      console.log( errors[i] );
    }
  }
  else {
    node.evaluate();
    console.log(JSON.stringify( node.getData("msaa", 0) ));
  }
}

FABRIC.close();
