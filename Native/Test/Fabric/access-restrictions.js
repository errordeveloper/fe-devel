FABRIC = require('Fabric').createClient();

op = FABRIC.DependencyGraph.createOperator("foo");
op.setEntryFunctionName("entry");
op.setSourceCode("\
operator entry( io Integer element, io Integer slice[] )\n\
{\n\
}\n\
");

binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout(["self.x", "self.x[]"]);

node = FABRIC.DependencyGraph.createNode( "bar" );
node.addMember( "x", "Integer" );
node.setCount( 2 );
node.bindings.append(binding);
/*
var errors = node.getErrors();
if ( errors.length > 0 ) {
  for ( var i in errors )
    console.log( errors[i] );
}
else {
  node.evaluate();
}
*/

FABRIC.flush();
