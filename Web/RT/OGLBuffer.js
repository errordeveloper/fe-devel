
//
// Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
//


FABRIC.define(["RT/Vec2",
               "RT/Vec3",
               "RT/Vec4",
               "RT/Mat33",
               "RT/Mat44",
               "RT/Color"], function() {


/**
 * Struct to store information about an opernGL buffer (VBO)
 * @constructor
 * @param {string} name The name of the VBO
 * @param {string} id The id of the VBO.
 * @param {string} dataType The state of the shader's value.
 */
FABRIC.RT.OGLBuffer = function(name, id, dataType, dynamic) {
  this.name = (name !== undefined) ? name : '';
  this.attributeID = (id !== undefined) ? id : 0;
  this.bufferID = 0;
  this.dynamic = (dynamic !== undefined) ? dynamic : false;
  this.reload = false;
  this.elementCount = 0;
  this.elementDataSize = dataType ? dataType.size : 0;
  this.numBufferElementComponents = this.elementDataSize/4;
  if(this.numBufferElementComponents > 4){
    throw "Buffers can only contain data types with of up to 128 byes per element";
  }
  this.bufferType = FABRIC.SceneGraph.OpenGLConstants ? FABRIC.SceneGraph.OpenGLConstants.GL_ARRAY_BUFFER : 0;
  this.bufferUsage = FABRIC.SceneGraph.OpenGLConstants ? FABRIC.SceneGraph.OpenGLConstants.GL_STATIC_DRAW : 0;
  this.bufferElementComponentType = FABRIC.SceneGraph.OpenGLConstants ? FABRIC.SceneGraph.OpenGLConstants.GL_FLOAT : 0;
};


FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('OGLBuffer', {
    members: {
      name: 'String',
      attributeID: 'Integer',
      bufferID: 'Integer',
      dynamic: 'Boolean',
      reload: 'Boolean',
      elementCount: 'Integer',
      elementDataSize: 'Integer',
      numBufferElementComponents: 'Integer',
      bufferType: 'Integer',
      bufferUsage: 'Integer',
      bufferElementComponentType: 'Integer'
    },
    constructor: FABRIC.RT.OGLBuffer,
    klBindings: {
      filename: 'OGLBuffer.kl',
      sourceCode: FABRIC.loadResourceURL('FABRIC_ROOT/RT/OGLBuffer.kl')
    }
  });
});


});
