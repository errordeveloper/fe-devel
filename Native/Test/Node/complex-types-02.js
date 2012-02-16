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
op.setEntryFunctionName("entry");
op.setSourceCode("\n\
operator entry( io MyStruct arg<>[] )\n\
{\n\
  report 'parent data: ' + arg;\n\
}\n\
");
var diagnostics = op.getDiagnostics();
if ( diagnostics.length > 0 ) {
  for ( var i in diagnostics ) {
    console.log( diagnostics[i].line + ": " + diagnostics[i].desc );
  }
  console.log( "Full code:" );
  console.log( op.getFullSourceCode() );
}
else {
  parentNode = FABRIC.DependencyGraph.createNode( "parentNode" );
  parentNode.addMember( "msa", "MyStruct[]" );
  parentNode.resize( 2 );
  parentNode.setData( "msa", 0, [new MyStruct( 42, 3.141 ), new MyStruct( 64, 5.67 ) ] );
  parentNode.setData( "msa", 1, [new MyStruct( 7, 2.718 )] );
  console.log(JSON.stringify( parentNode.getData("msa", 0) ));
  console.log(JSON.stringify( parentNode.getData("msa", 1) ));

	var binding = FABRIC.DG.createBinding();
	binding.setOperator(op);
	binding.setParameterLayout(["parent.msa<>"]);

  node = FABRIC.DependencyGraph.createNode( "node" );
  node.setDependency( parentNode, "parent" );
  node.bindings.append(binding);
	var errors = node.getErrors();
  if ( errors.length > 0 ) {
    for ( var i in errors ) {
      console.log( errors[i] );
    }
  }
  else {
    node.evaluate();
  }
}

FABRIC.close();
