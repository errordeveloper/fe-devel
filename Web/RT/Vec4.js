/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["RT/Math"], function() {

//determine if an object is a valid Vec4.
FABRIC.RT.isVec4 = function(t) {
  return t && t.getType &&
         t.getType() === 'FABRIC.RT.Vec4';
};

//Constructor:
//  Supported args:
//    (none)
//    x, y, z, t
//    Vec4
FABRIC.RT.Vec4 = function() {
  if (arguments.length == 4 &&
      FABRIC.RT.isScalar(arguments[0]) && 
      FABRIC.RT.isScalar(arguments[1]) && 
      FABRIC.RT.isScalar(arguments[2]) && 
      FABRIC.RT.isScalar(arguments[3]) ) {
    this.x = arguments[0];
    this.y = arguments[1];
    this.z = arguments[2];
    this.t = arguments[3];
  }
  else if (arguments.length == 1 && FABRIC.RT.isVec4(arguments[0])) {
    this.x = arguments[0].x;
    this.y = arguments[0].y;
    this.z = arguments[0].z;
    this.t = arguments[0].t;
  }
  else if (arguments.length == 0) {
    this.x = 0;
    this.y = 0;
    this.z = 0;
    this.t = 0;
  }
  else throw'new Vec4: invalid arguments';
  };

FABRIC.RT.Vec4.prototype = {

  //set: see constructor for supported args
  set: function() {
    FABRIC.RT.Vec4.apply(this, arguments);
    return this;
  },

  setNull: function(x, y, z, t) {
    this.x = 0; this.y = 0; this.z = 0; this.t = 0;
    return this;
  },

  equal: function(v) {
    var result = //JS bug: if the condition is directly returned it is wrong (??)
      this.x === v.x &&
      this.y === v.y &&
      this.z === v.z &&
      this.t === v.t;
    return result;
  },

  almostEqual: function(v, precision) {
    if (precision === undefined) {
      precision = Math.PRECISION;
    }
    var result = //JS bug: if the condition is directly returned it is wrong (??)
      (Math.abs(this.x - v.x) < precision) &&
      (Math.abs(this.y - v.y) < precision) &&
      (Math.abs(this.z - v.z) < precision) &&
      (Math.abs(this.t - v.t) < precision);
    return result;
  },

  component: function(i) {
    if(i == 0)
      return this.x;
    else if(i == 1)
      return this.y;
    else if(i == 2)
      return this.z;
    else
      return this.t;
  },

  setComponent: function(i, v) {
    if(i == 0)
      this.x = v;
    else if(i == 1)
      this.y = v;
    else if(i == 2)
      this.z = v;
    else
      this.t = v;
  },

  // Returns the result of adding the argument to the vector
  add: function(v) {
    return new FABRIC.RT.Vec4(this.x + v.x, this.y + v.y, this.z + v.z, this.t + v.t);
  },

  subtract: function(v) {
    return new FABRIC.RT.Vec4(this.x - v.x, this.y - v.y, this.z - v.z, this.t - v.t);
  },

  multiply: function(v) {
    return new FABRIC.RT.Vec4(this.x * v.x, this.y * v.y, this.z * v.z, this.t * v.t);
  },

  multiplyScalar: function(s) {
    return new FABRIC.RT.Vec4(this.x * s, this.y * s, this.z * s, this.t * s);
  },

  divide: function(v) {
    if( Math.verboseLogFunction ) {
      Math.checkDivisor(v.x, 'Vec4.divide v.x');
      Math.checkDivisor(v.y, 'Vec4.divide v.y');
      Math.checkDivisor(v.z, 'Vec4.divide v.z');
      Math.checkDivisor(v.t, 'Vec4.divide v.t');
    }
    return new FABRIC.RT.Vec4(this.x / v.x, this.y / v.y, this.z / v.z, this.t / v.t);
  },

  divideScalar: function(s) {
    Math.checkDivisor(s, 'Vec4.divideScalar');
    return this.multiplyScalar(1.0 / s);
  },
  
  premultiplyMat44: function(mat44) {
    return new FABRIC.RT.Vec4(
      mat44.row0.x * this.x + mat44.row1.x * this.y + mat44.row2.x * this.z + mat44.row3.x * this.t,
      mat44.row0.y * this.x + mat44.row1.y * this.y + mat44.row2.y * this.z + mat44.row3.y * this.t,
      mat44.row0.z * this.x + mat44.row1.z * this.y + mat44.row2.z * this.z + mat44.row3.z * this.t,
      mat44.row0.t * this.x + mat44.row1.t * this.y + mat44.row2.t * this.z + mat44.row3.t * this.t
    );
  },

  negate: function() {
    return new FABRIC.RT.Vec4(-this.x, - this.y, - this.z, - this.t);
  },

  inverse: function() {
    if( Math.verboseLogFunction ) {
      Math.checkDivisor(this.x, 'Vec4.inverse this.x');
      Math.checkDivisor(this.y, 'Vec4.inverse this.y');
      Math.checkDivisor(this.z, 'Vec4.inverse this.z');
      Math.checkDivisor(this.t, 'Vec4.inverse this.t');
    }
    return new FABRIC.RT.Vec4(1.0/this.x, 1.0/this.y, 1.0/this.z, 1.0/this.t);
  },

  dot: function(v) {
    return (this.x * v.x) + (this.y * v.y) + (this.z * v.z) + (this.t * v.t);
  },

  length: function() {
    return Math.sqrt(this.dot(this));
  },

  lengthSquared: function() {
    return this.dot(this);
  },

  unit: function() {
    var len = this.length();
    Math.checkDivisor(len, 'Vec4.unit');
    return this.divideScalar(len);
  },

  //Note: setUnit returns the previous length
  setUnit: function() {
    var len = this.length();
    Math.checkDivisor(len, 'Vec4.setUnit');
    var invLen = 1.0 / len;
    this.x *= invLen;
    this.y *= invLen;
    this.z *= invLen;
    this.t *= invLen;
    return len;
  },

  clamp: function(min, max) {
    return new FABRIC.RT.Vec4(
      (this.x < min.x ? min.x : (this.x > max.x ? max.x : this.x)),
      (this.y < min.y ? min.y : (this.y > max.y ? max.y : this.y)),
      (this.z < min.z ? min.z : (this.z > max.z ? max.z : this.z)),
      (this.t < min.t ? min.t : (this.t > max.t ? max.t : this.t))
    );
  },

  makeHomogeneousVec3: function() {
    if( this.t != 1.0 ) {
      Math.checkDivisor(this.t, 'Vec3.makeHomogeneousVec3');
      var invT = 1.0 / this.t;
      return new FABRIC.RT.Vec3(this.x * invT, this.y * invT, this.z * invT);
    }
    else
      return new FABRIC.RT.Vec3(this.x, this.y, this.z);
  },

  //Note: expects both vectors to be units (else use angleTo)
  unitsAngleTo: function(v) {
    var acosAngle = Math.clamp(this.dot(v), -1.0, 1.0);
    return Math.acos(acosAngle);
  },

  angleTo: function(v) {
    return this.unit().unitsAngleTo(v.unit());
  },

  distanceTo: function(other) {
    return this.subtract(other).length();
  },

  linearInterpolate: function(other, s) {
    return this.add(other.subtract(this).multiplyScalar(s));
  },

  clone: function() {
    return (new FABRIC.RT.Vec4(this.x, this.y, this.z, this.t));
  },

  toString: function() {
    return 'FABRIC.RT.Vec4(' + this.x + ',' + this.y + ',' + this.z + ',' + this.t + ')';
  },
  getType: function() {
    return 'FABRIC.RT.Vec4';
  }
};

//Vec4 constants
FABRIC.RT.Vec4.origin = new FABRIC.RT.Vec4(0, 0, 0, 0);
FABRIC.RT.Vec4.xAxis = new FABRIC.RT.Vec4(1, 0, 0, 0);
FABRIC.RT.Vec4.yAxis = new FABRIC.RT.Vec4(0, 1, 0, 0);
FABRIC.RT.Vec4.zAxis = new FABRIC.RT.Vec4(0, 0, 1, 0);
FABRIC.RT.Vec4.tAxis = new FABRIC.RT.Vec4(0, 0, 0, 1);

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('Vec4', {
    members: {
      x: 'Scalar', y: 'Scalar', z: 'Scalar', t: 'Scalar'
    },
    constructor: FABRIC.RT.Vec4,
    klBindings: {
      filename: 'Vec4.kl',
      sourceCode: FABRIC.loadResourceURL('FABRIC_ROOT/RT/Vec4.kl')
    }
  });
});



  return FABRIC.RT.Vec4;
});
