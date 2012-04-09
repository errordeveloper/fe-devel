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

Vec2.prototype = {
  sum: function() {
    return this.x + this.y;
  }
};

var desc = {
  members: { x:'Scalar', y:'Scalar' },
  constructor: Vec2,
  klBindings: {
    filename: "(inline)",
    sourceCode: "\
function Vec2(Scalar x, Scalar y)\n\
{\n\
  this.x = x;\n\
  this.y = y;\n\
}\n\
"
  }
};

FABRIC.RegisteredTypesManager.registerType( 'Vec2', desc );

var node = FABRIC.DG.createNode("foo");
node.addMember("bar", "Vec2");
node.setData("bar", {x:3.5});

FABRIC.flush();
FABRIC.close();
