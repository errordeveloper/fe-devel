/*;
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.;
 */;


fabricClient = require('Fabric').createClient();

try {
  fabricClient.RT.registerType( 'Foobar1', 'foobar' );
  fabricClient.flush();
}
catch (e) {
  console.log( 'error registering Foobar1: ' + e );
}

try {
  desc = {
    'members': 'x',
    'constructor': Object
  }
  fabricClient.RT.registerType( 'Foobar2', desc );
  fabricClient.flush();
}
catch (e) {
  console.log( 'error registering Foobar2: ' + e );
}

try {
  desc = {
    'constructor': Object
  }
  fabricClient.RT.registerType( 'Foobar3', desc );
  fabricClient.flush();
}
catch (e) {
  console.log( 'error registering Foobar3: ' + e );
}

desc = {
  'members': { 'x':'Scalar', 'y':'Scalar' }
}
fabricClient.RT.registerType( 'Foobar4', desc );
node = fabricClient.DG.createNode( 'node1' );
node.addMember( 'myFoo3', 'Foobar4' );
data = node.getData( 'myFoo3', 0 );
console.log( 'Foobar4: have x = ' + data.x + ', y = ' + data.y );

fabricClient.close();

