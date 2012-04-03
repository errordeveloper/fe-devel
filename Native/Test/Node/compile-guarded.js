var run = function( opts ) {
  c = require('Fabric').createClient( opts );

  n = c.DG.createNode( "foo" );
  n.addMember( "arr", "Size[]" );
  n.setData( "arr", 0, [ 1 ] );

  op = c.DG.createOperator( "op" );
  op.setSourceCode( "operator entry(Size arr[]) { arr[1]; } " );
  op.setEntryPoint( "entry" );

  b = c.DG.createBinding();
  b.setOperator( op );
  b.setParameterLayout( [ "self.arr" ] );

  n.bindings.append( b );

  try {
    n.evaluate();
  }
  catch ( e ) {
    console.log( e );
  }

  c.close();
};

run( { 'guarded': true } )
run()

