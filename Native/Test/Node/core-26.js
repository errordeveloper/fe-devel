/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var Vec2 = function( x, y ) {
  if ( typeof x === "number" && typeof y === "number" ) {
    this.x = x;
    this.y = y;
  }
  else if ( x === undefined && y === undefined ) {
    this.x = 0;
    this.y = 0;
  }
  else throw "new Vec2: invalid arguments";
};

var desc = {
  members: { x:'Scalar', y:'NotAType' },
  constructor: Vec2
};

FABRIC.RegisteredTypesManager.registerType( 'Vec2', desc );
FABRIC.flush();

FABRIC.close();
