
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

