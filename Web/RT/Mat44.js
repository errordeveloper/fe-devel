/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["RT/Math", "RT/Vec4"], function() {


//determine if an object is a valid Mat44.
FABRIC.RT.isMat44 = function(t) {
  return t && t.getType &&
         t.getType() === 'FABRIC.RT.Mat44';
};

//Constructor:
//  Supported args:
//    (none)
//    Vec4: row0, row1, row2, row3
//    16 scalars (_00, _01, ...)
//    Mat44
FABRIC.RT.Mat44 = function() {
  if (arguments.length == 4 &&
      FABRIC.RT.isVec4(arguments[0]) &&
      FABRIC.RT.isVec4(arguments[1]) &&
      FABRIC.RT.isVec4(arguments[2]) &&
      FABRIC.RT.isVec4(arguments[3])) {
    this.row0 = arguments[0].clone();
    this.row1 = arguments[1].clone();
    this.row2 = arguments[2].clone();
    this.row3 = arguments[3].clone();
  }
  else if (arguments.length == 16 &&
      FABRIC.RT.isScalar(arguments[0]) &&
      FABRIC.RT.isScalar(arguments[1]) &&
      FABRIC.RT.isScalar(arguments[2]) &&
      FABRIC.RT.isScalar(arguments[3]) &&
      FABRIC.RT.isScalar(arguments[4]) &&
      FABRIC.RT.isScalar(arguments[5]) &&
      FABRIC.RT.isScalar(arguments[6]) &&
      FABRIC.RT.isScalar(arguments[7]) &&
      FABRIC.RT.isScalar(arguments[8]) &&
      FABRIC.RT.isScalar(arguments[9]) &&
      FABRIC.RT.isScalar(arguments[10]) &&
      FABRIC.RT.isScalar(arguments[11]) &&
      FABRIC.RT.isScalar(arguments[12]) &&
      FABRIC.RT.isScalar(arguments[13]) &&
      FABRIC.RT.isScalar(arguments[14]) &&
      FABRIC.RT.isScalar(arguments[15])) {
    this.row0 = new FABRIC.RT.Vec4(arguments[0], arguments[1], arguments[2], arguments[3]);
    this.row1 = new FABRIC.RT.Vec4(arguments[4], arguments[5], arguments[6], arguments[7]);
    this.row2 = new FABRIC.RT.Vec4(arguments[8], arguments[9], arguments[10], arguments[11]);
    this.row3 = new FABRIC.RT.Vec4(arguments[12], arguments[13], arguments[14], arguments[15]);
  }
  else if (arguments.length == 1 && FABRIC.RT.isMat44(arguments[0])) {
    this.row0 = arguments[0].row0.clone();
    this.row1 = arguments[0].row1.clone();
    this.row2 = arguments[0].row2.clone();
    this.row3 = arguments[0].row3.clone();
  }
  else if (arguments.length == 0) {
    this.row0 = FABRIC.RT.Vec4.xAxis.clone();
    this.row1 = FABRIC.RT.Vec4.yAxis.clone();
    this.row2 = FABRIC.RT.Vec4.zAxis.clone();
    this.row3 = FABRIC.RT.Vec4.tAxis.clone();
  }
  else throw'new Mat44: invalid arguments';
};

FABRIC.RT.Mat44.prototype = {

  //set: see constructor for supported args
  set: function() {
    FABRIC.RT.Mat44.apply(this, arguments);
    return this;
  },

  setFromMat33: function(m) {
    this.row0.x = m.row0.x; this.row0.y = m.row0.y; this.row0.z = m.row0.z; this.row0.t = 0.0;
    this.row1.x = m.row1.x; this.row1.y = m.row1.y; this.row1.z = m.row1.z; this.row1.t = 0.0;
    this.row2.x = m.row2.x; this.row2.y = m.row2.y; this.row2.z = m.row2.z; this.row2.t = 0.0;
    this.row3.x = 0.0; this.row3.y = 0.0; this.row3.z = 0.0; this.row3.t = 1.0;
    return this;
  },

  setRows: function(row0, row1, row2, row3) {
    this.row0 = row0.clone();
    this.row1 = row1.clone();
    this.row2 = row2.clone();
    this.row3 = row3.clone();
    return this;
  },

  setColumns: function(col0, col1, col2, col3) {
    this.row0.x = col0.x; this.row0.y = col1.x; this.row0.z = col2.x; this.row0.t = col3.x;
    this.row1.x = col0.y; this.row1.y = col1.y; this.row1.z = col2.y; this.row1.t = col3.y;
    this.row2.x = col0.z; this.row2.y = col1.z; this.row2.z = col2.z; this.row2.t = col3.z;
    this.row3.x = col0.t; this.row3.y = col1.t; this.row3.z = col2.t; this.row3.t = col3.t;
    return this;
  },

  setNull: function() {
    this.row0 = FABRIC.RT.Vec4.origin.clone();
    this.row1 = FABRIC.RT.Vec4.origin.clone();
    this.row2 = FABRIC.RT.Vec4.origin.clone();
    this.row3 = FABRIC.RT.Vec4.origin.clone();
    return this;
  },

  setIdentity: function() {
    return this.set();
  },

  setDiagonal: function(v) {
    if( FABRIC.RT.isScalar(v) ) {
      this.row0.x = this.row1.y = this.row2.z = this.row3.t = v;
    } else {
      this.row0.x = v.x;
      this.row1.y = v.y;
      this.row2.z = v.z;
      if(v.t !== undefined)
        this.row3.t = v.t;
      else
        this.row3.t = 1.0;
    }
    return this;
  },

  equal: function(v) {
    return this.row0.equal(v.row0) && this.row1.equal(v.row1) && this.row2.equal(v.row2) && this.row3.equal(v.row3);
  },

  almostEqual: function(v, precision) {
    if (precision === undefined) {
      precision = Math.PRECISION;
    }
    return this.row0.almostEqual(v.row0, precision) && this.row1.almostEqual(v.row1, precision) && this.row2.almostEqual(v.row2, precision) && this.row3.almostEqual(v.row3, precision);
  },

  add: function(that) {
    return new FABRIC.RT.Mat44(
      this.row0.add(that.row0),
      this.row1.add(that.row1),
      this.row2.add(that.row2),
      this.row3.add(that.row3)
    );
  },

  subtract: function(that) {
    return new FABRIC.RT.Mat44(
      this.row0.subtract(that.row0),
      this.row1.subtract(that.row1),
      this.row2.subtract(that.row2),
      this.row3.subtract(that.row3)
    );
  },

  multiplyScalar: function(that) {
    return new FABRIC.RT.Mat44(
      this.row0.multiplyScalar(that),
      this.row1.multiplyScalar(that),
      this.row2.multiplyScalar(that),
      this.row3.multiplyScalar(that)
    );
  },

  multiplyVector: function(that) {
    if(that.t !== undefined) {
      return new FABRIC.RT.Vec4(
        this.row0.dot(that),
        this.row1.dot(that),
        this.row2.dot(that),
        this.row3.dot(that)
      );
    }
    else {
      var temp = this.multiplyVector(new FABRIC.RT.Vec4(that.x, that.y, that.z, 1.0));
      return temp.makeHomogeneousVec3();
    }
  },
  
  
  preMultiplyVector: function(that) {
    if(that.t !== undefined) {
      return new FABRIC.RT.Vec4(
        this.row0.x * that.x + this.row1.x * that.y + this.row2.x * that.z + this.row3.x * that.t,
        this.row0.y * that.x + this.row1.y * that.y + this.row2.y * that.z + this.row3.y * that.t,
        this.row0.z * that.x + this.row1.z * that.y + this.row2.z * that.z + this.row3.z * that.t,
        this.row0.t * that.x + this.row1.t * that.y + this.row2.t * that.z + this.row3.t * that.t
      );
    }
    else {
      var temp = this.multiplyVector(new FABRIC.RT.Vec4(that.x, that.y, that.z, 1.0));
      return temp.makeHomogeneousVec3();
    }
  },

  multiply: function(that) {
    return new FABRIC.RT.Mat44(
      this.row0.x * that.row0.x + this.row0.y * that.row1.x + this.row0.z * that.row2.x + this.row0.t * that.row3.x,
      this.row0.x * that.row0.y + this.row0.y * that.row1.y + this.row0.z * that.row2.y + this.row0.t * that.row3.y,
      this.row0.x * that.row0.z + this.row0.y * that.row1.z + this.row0.z * that.row2.z + this.row0.t * that.row3.z,
      this.row0.x * that.row0.t + this.row0.y * that.row1.t + this.row0.z * that.row2.t + this.row0.t * that.row3.t,
      //
      this.row1.x * that.row0.x + this.row1.y * that.row1.x + this.row1.z * that.row2.x + this.row1.t * that.row3.x,
      this.row1.x * that.row0.y + this.row1.y * that.row1.y + this.row1.z * that.row2.y + this.row1.t * that.row3.y,
      this.row1.x * that.row0.z + this.row1.y * that.row1.z + this.row1.z * that.row2.z + this.row1.t * that.row3.z,
      this.row1.x * that.row0.t + this.row1.y * that.row1.t + this.row1.z * that.row2.t + this.row1.t * that.row3.t,
      //
      this.row2.x * that.row0.x + this.row2.y * that.row1.x + this.row2.z * that.row2.x + this.row2.t * that.row3.x,
      this.row2.x * that.row0.y + this.row2.y * that.row1.y + this.row2.z * that.row2.y + this.row2.t * that.row3.y,
      this.row2.x * that.row0.z + this.row2.y * that.row1.z + this.row2.z * that.row2.z + this.row2.t * that.row3.z,
      this.row2.x * that.row0.t + this.row2.y * that.row1.t + this.row2.z * that.row2.t + this.row2.t * that.row3.t,
      //
      this.row3.x * that.row0.x + this.row3.y * that.row1.x + this.row3.z * that.row2.x + this.row3.t * that.row3.x,
      this.row3.x * that.row0.y + this.row3.y * that.row1.y + this.row3.z * that.row2.y + this.row3.t * that.row3.y,
      this.row3.x * that.row0.z + this.row3.y * that.row1.z + this.row3.z * that.row2.z + this.row3.t * that.row3.z,
      this.row3.x * that.row0.t + this.row3.y * that.row1.t + this.row3.z * that.row2.t + this.row3.t * that.row3.t
    );
  },

  divideScalar: function(s) {
    Math.checkDivisor(s, 'Mat44.divideScalar');
    return this.multiplyScalar(1.0 / s);
  },

  internalCompute: function(computeDeterminant, computeAdjoint) {
    result = {};
    var a0 = this.row0.x * this.row1.y - this.row0.y * this.row1.x;
    var a1 = this.row0.x * this.row1.z - this.row0.z * this.row1.x;
    var a2 = this.row0.x * this.row1.t - this.row0.t * this.row1.x;
    var a3 = this.row0.y * this.row1.z - this.row0.z * this.row1.y;
    var a4 = this.row0.y * this.row1.t - this.row0.t * this.row1.y;
    var a5 = this.row0.z * this.row1.t - this.row0.t * this.row1.z;
    var b0 = this.row2.x * this.row3.y - this.row2.y * this.row3.x;
    var b1 = this.row2.x * this.row3.z - this.row2.z * this.row3.x;
    var b2 = this.row2.x * this.row3.t - this.row2.t * this.row3.x;
    var b3 = this.row2.y * this.row3.z - this.row2.z * this.row3.y;
    var b4 = this.row2.y * this.row3.t - this.row2.t * this.row3.y;
    var b5 = this.row2.z * this.row3.t - this.row2.t * this.row3.z;

    if(computeDeterminant) {
      result.determinant = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    }

    if(computeAdjoint) {
      result.adjoint = new FABRIC.RT.Mat44(
        (this.row1.y * b5 - this.row1.z * b4 + this.row1.t * b3),
        (-this.row0.y * b5 + this.row0.z * b4 - this.row0.t * b3),
        (this.row3.y * a5 - this.row3.z * a4 + this.row3.t * a3),
        (-this.row2.y * a5 + this.row2.z * a4 - this.row2.t * a3),
        (-this.row1.x * b5 + this.row1.z * b2 - this.row1.t * b1),
        (this.row0.x * b5 - this.row0.z * b2 + this.row0.t * b1),
        (-this.row3.x * a5 + this.row3.z * a2 - this.row3.t * a1),
        (this.row2.x * a5 - this.row2.z * a2 + this.row2.t * a1),
        (this.row1.x * b4 - this.row1.y * b2 + this.row1.t * b0),
        (-this.row0.x * b4 + this.row0.y * b2 - this.row0.t * b0),
        (this.row3.x * a4 - this.row3.y * a2 + this.row3.t * a0),
        (-this.row2.x * a4 + this.row2.y * a2 - this.row2.t * a0),
        (-this.row1.x * b3 + this.row1.y * b1 - this.row1.z * b0),
        (this.row0.x * b3 - this.row0.y * b1 + this.row0.z * b0),
        (-this.row3.x * a3 + this.row3.y * a1 - this.row3.z * a0),
        (this.row2.x * a3 - this.row2.y * a1 + this.row2.z * a0)
      );
    }
    return result;
  },

  determinant: function() {
    return this.internalCompute(true, false).determinant;
  },

  adjoint: function() {
    return this.internalCompute(false, true).adjoint;
  },

  inverse: function() {
    var values = this.internalCompute(true, true);
    var det = values.determinant;
    if( Math.verboseLogFunction ) {
      Math.checkDivisor(det, 'Mat44.inverse');
    }
    return values.adjoint.divideScalar(det);
  },

  transpose: function() {
    return new FABRIC.RT.Mat44(
      this.row0.x, this.row1.x, this.row2.x, this.row3.x,
      this.row0.y, this.row1.y, this.row2.y, this.row3.y,
      this.row0.z, this.row1.z, this.row2.z, this.row3.z,
      this.row0.t, this.row1.t, this.row2.t, this.row3.t);
  },

  getTranslation: function () {
    return new FABRIC.RT.Vec3(this.row0.t, this.row1.t, this.row2.t);
  },
  setTranslation: function (tr) {
    this.row0.t = tr.x;
    this.row1.t = tr.y;
    this.row2.t = tr.z;
  },

  upperLeft: function() {
    var result = new FABRIC.RT.Mat33();

    result.row0.x = this.row0.x;
    result.row0.y = this.row0.y;
    result.row0.z = this.row0.z;

    result.row1.x = this.row1.x;
    result.row1.y = this.row1.y;
    result.row1.z = this.row1.z;

    result.row2.x = this.row2.x;
    result.row2.y = this.row2.y;
    result.row2.z = this.row2.z;

    return result;
  },

  clone: function() {
    return new FABRIC.RT.Mat44(this);
  },
  toString: function() {
    return 'FABRIC.RT.Mat44(' + this.row0.toString() + ',' + this.row1.toString() + ',' +
      this.row2.toString() + ',' + this.row3.toString() + ')';
  },
  getType: function() {
    return 'FABRIC.RT.Mat44';
  }
};

//Mat44 constants
FABRIC.RT.Mat44.identity = new FABRIC.RT.Mat44();

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('Mat44', {
    members: {
      row0: 'Vec4',
      row1: 'Vec4',
      row2: 'Vec4',
      row3: 'Vec4'
    },
    constructor: FABRIC.RT.Mat44,
    klBindings: {
      filename: 'Mat44.kl',
      sourceCode: FABRIC.loadResourceURL('FABRIC_ROOT/RT/Mat44.kl')
    }
  });
});

  return FABRIC.RT.Mat44;
});
