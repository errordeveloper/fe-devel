
var create_lots_nodes = function() {
  var iterations = 0;

  var runOneLoop = function() {
    iterations++;
    console.log('mem: ' + ( process.memoryUsage().rss / 1024.0 / 1024.0 ).toFixed( 2 ) + 'MB');
    console.log('starting loop ' + iterations + '...');
    c = require('Fabric').createClient();
    for (var i=0; i<10000; i++) {
      c.DG.createNode("foo"+i);
    }
    console.log('flushing...');
    c.flush();
    console.log('closing...');
    c.close();
    delete c;

    // we have to give Node time to garbage collect
    setTimeout( runOneLoop, 0 );
  };

  runOneLoop();
}

var pass_lots_data = function() {
  var c = require('Fabric').createClient();

  var iters = 32;
  var size = 2048;
  var num = 0;

  n = c.DG.createNode("test");
  n.addMember( "input", "String" );
  n.addMember( "output", "String" );
  n.resize( size );

  op = c.DG.createOperator( "testOp" );
  op.setEntryPoint( "entry" );
  op.setSourceCode( "operator entry( io String input, io String output ) { output = 'output: ' + input; }" );

  b = c.DG.createBinding();
  b.setOperator( op );
  b.setParameterLayout( [ "self.input", "self.output" ] );

  n.bindings.append( b );

  for (var i=0; i<iters; i++) {
    for (var j=0; j<size; j++) {
      n.setData( "input", j, "input"+num);
      num += 1;
    }

    n.evaluate();

    for (var j=0; j<size; j++) {
      x = n.getData( "output", i );
    }
  }

  c.close();
}

pass_lots_data();

