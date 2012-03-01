/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var Empty = function() {
};

Empty.prototype = {
};

var desc = {
  members: {},
  constructor: Empty
};

FABRIC.RegisteredTypesManager.registerType( 'Empty', desc );
console.log(JSON.stringify(FABRIC.RT.getRegisteredTypes()['Empty']));

FABRIC.close();
