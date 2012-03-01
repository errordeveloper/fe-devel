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
console.log(JSON.stringify(FABRIC.RT.getRegisteredTypes()['Vec2']));

var node = FABRIC.DependencyGraph.createNode("foo");
node.addMember( 'vec2', 'Vec2' );
node.setData( 'vec2', 0, new Vec2( 5.6, 4.3 ) );
var data = node.getData("vec2", 0);
console.log( JSON.stringify(data) );
console.log(data.sum());

var op = FABRIC.DG.createOperator("op");
op.setSourceCode("(inline)", "require Vec2; operator entry( io Vec2 vec2 ) { vec2 = Vec2(8.9, 2.3); }");
op.setEntryFunctionName("entry");
if (op.getErrors().length > 0) {
  console.log(op.getErrors());
  if (op.getDiagnostics().length > 0)
    console.log(op.getDiagnostics());
}

var binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout(["self.vec2"]);

node.bindings.append(binding);
node.evaluate();
var data = node.getData("vec2", 0);
console.log( JSON.stringify(data) );
console.log(data.sum());

// test new registerType 'members' layout
var desc2 = {
  members: [ { x:'Scalar' }, { y:'Scalar' } ],
  constructor: Vec2
};
FABRIC.RegisteredTypesManager.registerType( 'Vec2a', desc2 );
console.log(JSON.stringify(FABRIC.RT.getRegisteredTypes()['Vec2a']));

var ComplexType = function() {
  var vec2 = new Vec2(3.14, 2.71);
  var a2_vec2 = [vec2,vec2];
  var a_a2_vec2 = [a2_vec2,a2_vec2,a2_vec2];
  var a2_a_a2_vec2 = [a_a2_vec2,a_a2_vec2];
  var a_a2_a_a2_vec2 = [a2_a_a2_vec2];
  this.member = a_a2_a_a2_vec2;
};
var complexTypeDesc = {
  members: { member: 'Vec2[][2][][2]' },
  constructor: ComplexType,
};
FABRIC.RT.registerType('ComplexType', complexTypeDesc);
console.log(JSON.stringify(FABRIC.RT.getRegisteredTypes()['ComplexType'].defaultValue));
node.addMember( 'ct', 'ComplexType' );
console.log(JSON.stringify( node.getData( 'ct', 0 ) ));

FABRIC.close();
