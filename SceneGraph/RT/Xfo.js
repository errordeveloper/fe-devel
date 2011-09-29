
//
// Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
//

//determine if an object is a valid Xfo.
FABRIC.RT.isXfoOrder = function(t) {
  return t && t.getType &&
         t.getType() === 'FABRIC.RT.Xfo';
};

//Constructor:
//  Supported args:
//    (none)
//    {ori, tr, sc}
//    Vec(tr)
//    Vec3(tr), Quat
//    Vec3(tr), Quat, Vec3(sc)
//    Xfo
FABRIC.RT.Xfo = function() {
  var good = true;
  //First, check if we have ([ori], [tr], [sc])
  if (arguments[0]) {
    if( FABRIC.RT.isVec3(arguments[0])) {
      this.tr = arguments[0].clone();
    }
    else
      good = false;
  }
  else {
      this.tr = new FABRIC.RT.Vec3();
  }
  if (good && arguments[1]) {
    if( FABRIC.RT.isQuat(arguments[1])) {
      this.ori = arguments[1].clone();
    }
    else
      good = false;
  }
  else {
      this.ori = new FABRIC.RT.Quat();
  }
  if (good && arguments[2]) {
    if( FABRIC.RT.isVec3(arguments[2])) {
      this.sc = arguments[2].clone();
    }
    else
      good = false;
  }
  else {
      this.sc = new FABRIC.RT.Vec3(1, 1, 1);
  }
  //Second, check if we have ({[ori], [tr], [sc]})
  if(arguments.length == 1 && typeof arguments[0] === 'object' && !good) {
    good = true;
    var obj = arguments[0];
    if (obj.ori) {
      if( FABRIC.RT.isQuat(obj.ori)) {
        this.ori = obj.ori.clone();
      }
      else
        good = false;
    }
    else {
        this.ori = new FABRIC.RT.Quat();
    }
    if (obj.tr) {
      if( FABRIC.RT.isVec3(obj.tr)) {
        this.tr = obj.tr.clone();
      }
      else
        good = false;
    }
    else {
        this.tr = new FABRIC.RT.Vec3();
    }
    if (obj.sc) {
      if( FABRIC.RT.isVec3(obj.sc)) {
        this.sc = obj.sc.clone();
      }
      else
        good = false;
    }
    else {
        this.sc = new FABRIC.RT.Vec3(1, 1, 1);
    }
  }
  if( !good )
    throw'Xfo: invalid arguments';
};

FABRIC.RT.Xfo.prototype = {

  //set: see constructor for supported args
  set: function() {
    FABRIC.RT.Xfo.apply(this, arguments);
    return this;
  },

  setIdentity: function() {
    this.set();
  },

  setFromMat44: function(m) {
    if (abs(1.0 - m.row3.t) > 0.001) {
      Math.reportWarning('Mat44.setFromMat44: Cannot handle denormalized matrices: ' + m.row3.t);
      this.setIdentity();
      return;
    }

    if (m.row3.x != 0.0 || m.row3.y != 0.0 || m.row3.z != 0.0) {
      Math.reportWarning('Mat44.setFromMat44: Cannot handle perspective projection matrices');
      this.setIdentity();
      return;
    }

    // We're going out on a limb and assuming this is a
    // straight homogenous transformation matrix. No
    // singularities, denormilizations, or projections.
    this.tr.x = m.row3.x;
    this.tr.y = m.row3.y;
    this.tr.z = m.row3.z;

    var mat33 = new FABRIC.RT.Mat33(
                  m.row0.x, m.row0.y, m.row0.z,
                  m.row1.x, m.row1.y, m.row1.z,
                  m.row2.x, m.row2.y, m.row2.z);

    // Grab the X scale and normalize the first row
    this.sc.x = mat33.row0.length();
    Math.checkDivisor(this.sc.x, 'Xfo.setFromMat44: Matrix is singular');
    mat33.row0.divideScalar(this.sc.x);

    // Make the 2nd row orthogonal to the 1st
    mat33.row1 = mat33.row1.subtract(mat33.row0.multiply(mat33.row0.dot(mat33.row1)));

    // Grab the Y scale and normalize the 2nd row
    this.sc.y = mat33.row1.length();
    Math.checkDivisor(this.sc.y, 'Xfo.setFromMat44: Matrix is singular');
    mat33.row1.divideScalar(this.sc.y);

    // Make the 3rd row orthogonal to the 1st and 2nd
    mat33.row2 = mat33.row2.subtract(mat33.row0.multiply(mat33.row0.dot(mat33.row2)));
    mat33.row2 = mat33.row2.subtract(mat33.row1.multiply(mat33.row1.dot(mat33.row2)));

    // Grab the Y scale and normalize the 2nd row
    this.sc.z = mat33.row2.length();
    Math.checkDivisor(this.sc.z, 'Xfo.setFromMat44: Matrix is singular');
    mat33.row2.divideScalar(this.sc.z);

    this.ori.setFromMat33(mat33);
  },

  toMat44: function() {
    var scl = new FABRIC.RT.Mat44(),
      rot = new FABRIC.RT.Mat44(),
      trn = new FABRIC.RT.Mat44(),
      q = this.ori;

    scl.setDiagonal(FABRIC.RT.vec4(this.sc.x, this.sc.y, this.sc.z, 1.0));

    rot.row0.x = 1.0 - 2.0 * (q.v.y * q.v.y + q.v.z * q.v.z);
    rot.row0.y = 2.0 * (q.v.x * q.v.y - q.v.z * q.w);
    rot.row0.z = 2.0 * (q.v.x * q.v.z + q.v.y * q.w);

    rot.row1.x = 2.0 * (q.v.x * q.v.y + q.v.z * q.w);
    rot.row1.y = 1.0 - 2.0 * (q.v.x * q.v.x + q.v.z * q.v.z);
    rot.row1.z = 2.0 * (q.v.y * q.v.z - q.v.x * q.w);

    rot.row2.x = 2.0 * (q.v.x * q.v.z - q.v.y * q.w);
    rot.row2.y = 2.0 * (q.v.y * q.v.z + q.v.x * q.w);
    rot.row2.z = 1.0 - 2.0 * (q.v.x * q.v.x + q.v.y * q.v.y);

    rot.row3.t = 1.0;

    trn.row0.t = this.tr.x;
    trn.row1.t = this.tr.y;
    trn.row2.t = this.tr.z;

    return trn.multiply(rot.multiply(scl));
  },

  multiply: function(xf) {
    var result = new FABRIC.RT.Xfo();
    result.sc = this.sc.multiply(xf.sc);
    result.ori = xf.ori.multiply(this.ori);
    //  result.ori = this.ori.multiply(xf.ori);
    result.tr = this.sc.multiply(xf.tr);
    result.tr = this.ori.rotateVector(result.tr);
    result.tr = this.tr.add(result.tr);
    return result;
  },

  inverse: function() {
    var result = new FABRIC.RT.Xfo();
    result.sc = this.sc.invert();
    result.ori = this.ori.invert();
    result.tr = this.tr.negate();
    return result;
  },

  transformVector: function(v) {
    return this.tr.add(this.ori.rotateVector(this.sc.multiply(v)));
  },

  clone: function() {
    var newXfo = new FABRIC.RT.Xfo;
    newXfo.ori = this.ori.clone();
    newXfo.tr = this.tr.clone();
    newXfo.sc = this.sc.clone();
    return newXfo;
  },

  toString: function() {
    return 'FABRIC.RT.xfo(' + this.ori.toString() + ',' + this.tr.toString() + ',' + this.sc.toString() + ')';
  },

  getType: function() {
    return 'FABRIC.RT.Xfo';
  },

  displayGUI: function($parentDiv, changeHandlerFn) {
    var val = this;
    var fn = function() {
      changeHandlerFn(val);
    };
    var oriRefreshFn = this.ori.displayGUI($parentDiv, fn); $parentDiv.append($('<br/>'));
    var trRefreshFn = this.tr.displayGUI($parentDiv, fn); $parentDiv.append($('<br/>'));
    var scRefreshFn = this.sc.displayGUI($parentDiv, fn);
    var refreshFn = function(val) {
      oriRefreshFn(val.ori);
      trRefreshFn(val.tr);
      scRefreshFn(val.sc);
    };
    return refreshFn;
  }
};

/**
 * Overloaded Constructor for a xfo object.
 * @return {object} The xfo object.
 */
FABRIC.RT.xfo = function() {
  // The following is a bit of a hack. Not sure if we can combine new and apply.
  if (arguments.length === 0) return new FABRIC.RT.Xfo();
  if (arguments.length === 1) return new FABRIC.RT.Xfo(arguments[0]);
  if (arguments.length === 2) return new FABRIC.RT.Xfo(arguments[0], arguments[1]);
  if (arguments.length === 3) return new FABRIC.RT.Xfo(arguments[0], arguments[1], arguments[2]);
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('Xfo', {
    members: {
      ori: 'Quat', tr: 'Vec3', sc: 'Vec3'
    },
    constructor: FABRIC.RT.Xfo,
    klBindings: {
      filename: 'Xfo.kl',
      sourceCode: FABRIC.loadResourceURL('FABRIC_ROOT/SceneGraph/RT/Xfo.kl')
    }
  });
});
