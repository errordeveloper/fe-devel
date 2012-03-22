/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Geometry"], function() {
  
FABRIC.RT.BulletWorld = function(options) {
  if(!options)
    options = {};
  this.localData = null;
  this.gravity = options.gravity ? options.gravity : new FABRIC.RT.Vec3(0,-40,0);
  this.step = 0;
  this.substeps = options.substeps ? options.substeps : 3;
  this.hit = false;
  this.hitPosition = new FABRIC.RT.Vec3(0,0,0);
  this.hitNormal= new FABRIC.RT.Vec3(0,1,0);
};

FABRIC.RT.BulletWorld.prototype = {
};


FABRIC.RT.BulletSoftBody = function( options ) {
  if(!options)
    options = {};
  this.localData = null;
  this.name = options.name ? options.name : 'SoftBody';
  this.transform = options.transform ? options.transform : FABRIC.RT.xfo();
  this.clusters = options.clusters ? options.clusters : -1;
  this.constraints = options.constraints ? options.constraints : 2;
  this.mass = options.mass ? options.mass : 1.0;
  this.stiffness = options.stiffness ? options.stiffness : 0.05;
  this.friction = options.friction ? options.friction : 0.5;
  this.conservation = options.conservation ? options.conservation : 0.0;
  this.pressure = options.pressure ? options.pressure : 0.0;
  this.recover = options.recover ? options.recover : 0.0;
  this.trianglesNode = options.trianglesNode ? options.trianglesNode : undefined;
};


FABRIC.RT.BulletSoftBody.prototype = {
};


  var BulletShapeTypes = {
    BULLET_BOX_SHAPE: 0,
    BULLET_CONVEX_HULL_SHAPE: 4,
    BULLET_SPHERE_SHAPE: 8,
    BULLET_CAPSULE_SHAPE: 10,
    BULLET_CONE_SHAPE: 11,
    BULLET_CYLINDER_SHAPE: 13,
    BULLET_TRIANGLEMESH_SHAPE: 21,
    BULLET_GIMPACT_SHAPE: 25,
    BULLET_PLANE_SHAPE: 28,
    BULLET_COMPOUND_SHAPE: 31,
    BULLET_SOFTBODY_SHAPE: 32
  };
  
FABRIC.RT.BulletShape = function(options) {
  if(!options)
    options = {};
  this.localData = null;
  this.name = options.name ? options.name : 'Shape';
  this.type = options.type ? options.type : -1;
  this.parameters = [];
  this.vertices = [];
  this.indices = [];
};

FABRIC.RT.BulletShape.prototype = {
};

FABRIC.RT.BulletShape.createBox = function(halfExtends) {
  if(halfExtends == undefined) {
    halfExtends = new FABRIC.RT.Vec3(1.0,1.0,1.0);
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_BOX_SHAPE;
  shape.parameters.push(halfExtends.x);
  shape.parameters.push(halfExtends.y);
  shape.parameters.push(halfExtends.z);
  return shape;
};

FABRIC.RT.BulletShape.createSphere = function(radius) {
  if(radius == undefined) {
    radius = 1.0;
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_SPHERE_SHAPE;
  shape.parameters.push(radius);
  return shape;
};

FABRIC.RT.BulletShape.createCylinder = function(radius,height) {
  if(radius == undefined) {
    radius = 0.5;
  }
  if(height == undefined) {
    height = 1.0;
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_CYLINDER_SHAPE;
  shape.parameters.push(radius);
  shape.parameters.push(height * 0.5);
  return shape;
};

FABRIC.RT.BulletShape.createPlane = function(normal) {
  if(normal == undefined) {
    normal = new FABRIC.RT.Vec3(0.0,1.0,0.0);
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_PLANE_SHAPE;
  shape.parameters.push(normal.x);
  shape.parameters.push(normal.y);
  shape.parameters.push(normal.z);
  shape.parameters.push(0.0);
  return shape;
};

FABRIC.RT.BulletShape.createConvexHull = function(geometryNode) {
  if(geometryNode == undefined || !geometryNode.isTypeOf('Geometry')) {
    throw('You need to specify the geometryNode for createConvexHull.');
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_CONVEX_HULL_SHAPE;
  shape.geometryNode = geometryNode;
  return shape;
};

FABRIC.RT.BulletShape.createTriangleMesh = function(geometryNode) {
  if(geometryNode == undefined || !geometryNode.isTypeOf('Geometry')) {
    throw('You need to specify the geometryNode for createTriangleMesh.');
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_TRIANGLEMESH_SHAPE;
  shape.geometryNode = geometryNode;
  return shape;
};

FABRIC.RT.BulletShape.createGImpact = function(geometryNode) {
  if(geometryNode == undefined || !geometryNode.isTypeOf('Geometry')) {
    throw('You need to specify the geometryNode for createGImpact.');
  }
  var shape = new FABRIC.RT.BulletShape();
  shape.type = BulletShapeTypes.BULLET_GIMPACT_SHAPE;
  shape.geometryNode = geometryNode;
  return shape;
};



FABRIC.RT.BulletRigidBody = function(options) {
  if(options == undefined)
    options = {};
  this.localData = null;
  this.name = options.name ? options.name : 'RigidBody';
  this.transform = options.transform != undefined ? options.transform: FABRIC.RT.xfo();
  this.mass = options.mass != undefined ? options.mass: 1.0;
  this.friction = options.friction != undefined ? options.friction: 0.9;
  this.restitution = options.restitution != undefined ? options.restitution: 0.0;
};

FABRIC.RT.BulletRigidBody.prototype = {
};

FABRIC.RT.BulletForce = function(options) {
  if(!options)
    options = {};
  this.name = options.name ? options.name : 'Force';
  this.origin = options.origin != undefined ? options.origin : new FABRIC.RT.Vec3(0,0,0);
  this.direction = options.direction != undefined ? options.direction : new FABRIC.RT.Vec3(0,1,0);
  this.radius = options.radius != undefined ? options.radius : 1.5;
  this.factor = options.factor != undefined ? options.factor : 100.0;
  this.useTorque = options.useTorque != undefined ? options.useTorque : true;
  this.useFalloff = options.useFalloff != undefined ? options.useFalloff : true;
  this.enabled = options.enabled != undefined ? options.enabled : true;
  this.autoDisable = options.autoDisable != undefined ? options.autoDisable : false;
};

FABRIC.RT.BulletForce.prototype = {
};

// constants
var BulletConstraintTypes = {
  BULLET_POINT2POINT_CONSTRAINT: 3,
  BULLET_HINGE_CONSTRAINT: 4,
  BULLET_SLIDER_CONSTRAINT: 7
}

FABRIC.RT.BulletConstraint = function(options) {
  if(!options)
    options = {};
  this.localData = null;
  this.bodyLocalDataA = null;
  this.bodyLocalDataB = null;
  this.type = -1;
  this.name = options.name ? options.name : 'Constraint';
  this.pivotA = options.pivotA ? options.pivotA : FABRIC.RT.xfo();
  this.pivotB = options.pivotB ? options.pivotB : FABRIC.RT.xfo();
  this.nameA = options.nameA ? options.nameA : '';
  this.nameB = options.nameB ? options.nameB : '';
  this.indexA = options.indexA != undefined ? options.indexA : 0;
  this.indexB = options.indexB != undefined ? options.indexB : 0;
  this.parameters = [];
};


FABRIC.RT.BulletConstraint.prototype = {
};

FABRIC.RT.BulletConstraint.createPoint2Point = function(options) {
  if(!options)
    options = {};
  var constraint = new FABRIC.RT.BulletConstraint(options);
  constraint.type = BulletConstraintTypes.BULLET_POINT2POINT_CONSTRAINT;
  return constraint;
};

FABRIC.RT.BulletConstraint.createHinge = function(options) {
  if(!options)
    options = {};
  var constraint = new FABRIC.RT.BulletConstraint(options);
  constraint.type = BulletConstraintTypes.BULLET_HINGE_CONSTRAINT;
  return constraint;
};

FABRIC.RT.BulletConstraint.createSlider = function(options) {
  if(!options)
    options = {};
  var constraint = new FABRIC.RT.BulletConstraint(options);
  constraint.type = BulletConstraintTypes.BULLET_SLIDER_CONSTRAINT;
  return constraint;
};

FABRIC.RT.BulletAnchor = function(options) {
  if(!options)
    options = {};
  this.localData = null;
  this.rigidBodyLocalData = null;
  this.softBodyLocalData = null;
  this.name = options.name ? options.name : 'Anchor';
  this.rigidBodyIndex = options.rigidBodyIndex != undefined ? options.rigidBodyIndex : 0;
  this.softBodyNodeIndices = options.softBodyNodeIndices != undefined ? options.softBodyNodeIndices : [];
  this.disableCollision = options.disableCollision != undefined ? options.disableCollision : true;
};

FABRIC.RT.BulletAnchor.prototype = {
};

FABRIC.SceneGraph.registerNodeType('BulletWorldNode', {
  briefDesc: 'The BulletWorldNode represents a bullet physics simulation world.',
  detailedDesc: 'The BulletWorldNode represents a bullet physics simulation world.',
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      createGroundPlane: true,
      createGroundPlaneGrid: true,
      groundPlaneSize: 40.0,
      connectToSceneTime: true,
      gravity: new FABRIC.RT.Vec3(0,-40,0),
      substeps: 3
    });
    
    // create the bullet node
    var bulletWorldNode = scene.constructNode('SceneGraphNode', {name: 'BulletWorldNode'});
    var dgnode = bulletWorldNode.constructDGNode('DGNode');
    dgnode.addMember('world', 'BulletWorld', new FABRIC.RT.BulletWorld(options));
    dgnode.addMember('prevTime', 'Scalar');

    // create the dgnodes to store shapes and bodies
    var shapedgnode = bulletWorldNode.constructDGNode('ShapeDGNode');
    var rbddgnode = bulletWorldNode.constructDGNode('RbdDGNode');
    var sbddgnode = bulletWorldNode.constructDGNode('SbdDGNode');
    shapedgnode.setDependency(dgnode,'simulation');
    rbddgnode.setDependency(dgnode,'simulation');
    rbddgnode.setDependency(shapedgnode,'shapes');
    sbddgnode.setDependency(dgnode,'simulation');
    sbddgnode.setDependency(rbddgnode,'rigidBodies');
    
    // create world init operator
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'createBulletWorld',
      parameterLayout: [
        'self.world'
      ],
      entryPoint: 'createBulletWorld',
      srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
    }));

    bulletWorldNode.pub.addShape = function(shapeName,shape) {
      if(shapeName == undefined){
        throw('You need to specify a shapeName when calling addShape!');
      }
      if(shape == undefined){
        throw('You need to specify a shape when calling addShape!');
      }
      shape.name = shapeName;
      shapedgnode.addMember(shapeName+'Shape', 'BulletShape', shape);

      // copy the points for convex hulls
      if(shape.type == BulletShapeTypes.BULLET_CONVEX_HULL_SHAPE) {
        if(!shape.geometryNode){
          throw('You need to specify geometryNode for a convex hull shape!');
        }
        // create rigid body operator
        shapedgnode.setDependency(scene.getPrivateInterface(shape.geometryNode).getAttributesDGNode(),shapeName+"Shape_attributes");
        shapedgnode.bindings.append(scene.constructOperator({
          operatorName: 'copyShapeVertices',
          parameterLayout: [
            'self.'+shapeName+'Shape',
            shapeName+"Shape_attributes.positions<>"
          ],
          entryPoint: 'copyShapeVertices',
          srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
        }));
      }

      // copy the points for convex hulls
      if(shape.type == BulletShapeTypes.BULLET_GIMPACT_SHAPE || shape.type == BulletShapeTypes.BULLET_TRIANGLEMESH_SHAPE)
      {
        if(!shape.geometryNode)
          throw('You need to specify geometryNode for a gimpact or trianglemeshshape shape!')
          
        // create rigid body operator
        shapedgnode.setDependency(scene.getPrivateInterface(shape.geometryNode).getAttributesDGNode(),shapeName+"Shape_attributes");
        shapedgnode.setDependency(scene.getPrivateInterface(shape.geometryNode).getUniformsDGNode(),shapeName+"Shape_uniforms");
        shapedgnode.bindings.append(scene.constructOperator({
          operatorName: 'copyShapeVertices',
          parameterLayout: [
            'self.'+shapeName+'Shape',
            shapeName+"Shape_attributes.positions<>"
          ],
          entryPoint: 'copyShapeVertices',
          srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
        }));
        shapedgnode.bindings.append(scene.constructOperator({
          operatorName: 'copyShapeIndices',
          parameterLayout: [
            'self.'+shapeName+'Shape',
            shapeName+"Shape_uniforms.indices"
          ],
          entryPoint: 'copyShapeIndices',
          srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
        }));
      }

      // create rigid body operator
      shapedgnode.bindings.append(scene.constructOperator({
        operatorName: 'createBulletShape',
        parameterLayout: [
          'self.'+shapeName+'Shape'
        ],
        entryPoint: 'createBulletShape',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    };

    bulletWorldNode.pub.addRigidBody = function(bodyName,body,shapeName) {
      if(bodyName == undefined){
        throw('You need to specify a bodyName when calling addRigidBody!');
      }
      if(body == undefined){
        throw('You need to specify a body when calling addRigidBody!');
      }
      if(shapeName == undefined){
        throw('You need to specify a shapeName when calling addRigidBody!');
      }
      // check if we are dealing with an array
      var i, isArray = (body.constructor.toString().indexOf("Array") != -1);
      if(isArray) {
        for(i=0; i < body.length; i++) {
          body[i].name = bodyName;
        }
      }
      else {
        body.name = bodyName;
      }
      if(!isArray) body = [body];

      // check if the shape is a collision surface (turn passive if so)
      var shape = shapedgnode.getData(shapeName+'Shape',0);
      if(shape.type == BulletShapeTypes.BULLET_TRIANGLEMESH_SHAPE) {
        for(var i=0;i<body.length;i++)
          body[i].mass = 0.0;
      }

      rbddgnode.addMember(bodyName+'Rbd', 'BulletRigidBody[]', body);

      // create rigid body operator
      rbddgnode.bindings.append(scene.constructOperator({
        operatorName: 'createBulletRigidBody',
        parameterLayout: [
          'simulation.world',
          'shapes.'+shapeName+'Shape',
          'self.'+bodyName+'Rbd'
        ],
        entryPoint: 'createBulletRigidBody',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    };

    var rbdInitialTransforms = {};
    bulletWorldNode.pub.setRigidBodyInitialTransform = function(bodyName, transform) {
      if(!rbdInitialTransforms[bodyName]) {
        rbdInitialTransforms[bodyName] = bodyName+'Transform';
        rbddgnode.addMember(rbdInitialTransforms[bodyName],'Xfo[]');
        rbddgnode.bindings.append(scene.constructOperator({
          operatorName: 'setBulletRigidBodyTransform',
          parameterLayout: [
            'self.'+bodyName+'Rbd',
            'self.'+rbdInitialTransforms[bodyName],
            'simulation.prevTime'
          ],
          entryPoint: 'setBulletRigidBodyTransform',
          srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
        }));
      }
      var isArray = transform.constructor.toString().indexOf("Array") != -1;
      var transforms = isArray ? transform : [transform];
      rbddgnode.setData(rbdInitialTransforms[bodyName], 0, transforms);
    };
    
    bulletWorldNode.pub.getRigidBodyInitialTransform = function(bodyName) {
      var i, body, result = [];
      if(!rbdInitialTransforms[bodyName]) {
        body = rbddgnode.getData(bodyName+'Rbd', 0);
        for(i=0; i < body.length; i++){
          result.push(new FABRIC.RT.Xfo({
            sc: new FABRIC.RT.Vec3(
              parseFloat(body[i].transform.sc.x),
              parseFloat(body[i].transform.sc.y),
              parseFloat(body[i].transform.sc.z)
            ),
            ori: new FABRIC.RT.Quat(
              parseFloat(body[i].transform.ori.w),
              new FABRIC.RT.Vec3(
                parseFloat(body[i].transform.ori.v.x),
                parseFloat(body[i].transform.ori.v.y),
                parseFloat(body[i].transform.ori.v.z)
            )),
            tr: new FABRIC.RT.Vec3(
              parseFloat(body[i].transform.tr.x),
              parseFloat(body[i].transform.tr.y),
              parseFloat(body[i].transform.tr.z)
            )
          }));
        }
      }
      else {
        result = rbddgnode.getData(bodyName+'Transform',0);
      }
      return result;
    };

    bulletWorldNode.pub.addSoftBody = function(bodyName,body) {
      if(bodyName == undefined){
        throw('You need to specify a bodyName when calling addSoftbody!');
      }
      if(body == undefined){
        throw('You need to specify a body when calling addSoftbody!');
      }
      if(body.trianglesNode == undefined){
        throw('You need to specify a trianglesNode for softbody when calling addSoftbody!');
      }

      body.name = bodyName;
      sbddgnode.addMember(bodyName+'Sbd', 'BulletSoftBody', body);
      
      var trianglesNode = scene.getPrivateInterface(body.trianglesNode);
      sbddgnode.setDependency(trianglesNode.getAttributesDGNode(),bodyName+"_Attributes");
      sbddgnode.setDependency(trianglesNode.getUniformsDGNode(),bodyName+"_Uniforms");

      var dataCopy = scene.constructNode('GeometryDataCopy', {baseGeometryNode: trianglesNode.pub} );
      dataCopy.pub.addVertexAttributeValue('positions', 'Vec3', { genVBO:true, dynamic:true } );
      dataCopy.pub.addVertexAttributeValue('normals', 'Vec3', { genVBO:true, dynamic:true } );
      dataCopy.getAttributesDGNode().setDependency(sbddgnode,'softbody');

      // create the create softbody operator
      sbddgnode.bindings.append(scene.constructOperator({
        operatorName: 'createBulletSoftBody',
        parameterLayout: [
          'simulation.world',
          'self.'+bodyName+'Sbd',
          bodyName+'_Attributes.positions<>',
          bodyName+'_Attributes.normals<>',
          bodyName+'_Uniforms.indices'
        ],
        entryPoint: 'createBulletSoftBody',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));

      // create the softbody position operator
       dataCopy.getAttributesDGNode().bindings.append(scene.constructOperator({
        operatorName: 'getBulletSoftBodyPosition',
        parameterLayout: [
          'self.index',
          'softbody.'+bodyName+'Sbd',
          'self.positions',
          'self.normals'
        ],
        entryPoint: 'getBulletSoftBodyPosition',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
      
      return dataCopy.pub;
    };
    
    bulletWorldNode.pub.addConstraint = function(constraintName,constraint,bodyNameA,bodyNameB) {
      if(constraintName == undefined){
        throw('You need to specify a constraintName when calling addConstraint!');
      }
      if(constraint == undefined){
        throw('You need to specify a constraint when calling addConstraint!');
      }
      if(bodyNameA == undefined){
        throw('You need to specify a bodyNameA when calling addConstraint!');
      }
      if(bodyNameB == undefined){
        throw('You need to specify a bodyNameB when calling addConstraint!');
      }
      // check if we are dealing with an array
      var i, isArray = constraint.constructor.toString().indexOf("Array") != -1;
      if(isArray) {
        for(i=0; i<constraint.length; i++) {
          constraint[i].name = constraintName;
        }
      }
      else {
        constraint.name = constraintName;
      }
      rbddgnode.addMember(constraintName, 'BulletConstraint[]', isArray ? constraint : [constraint]);

      // create rigid body operator
      rbddgnode.bindings.append(scene.constructOperator({
        operatorName: 'createBulletConstraint',
        parameterLayout: [
          'simulation.world',
          'self.'+constraintName,
          'self.'+bodyNameA+'Rbd',
          'self.'+bodyNameB+'Rbd'
        ],
        entryPoint: 'createBulletConstraint',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    };

    bulletWorldNode.pub.addAnchor = function(anchorName,anchor,rigidBodyName,softBodyName) {
      if(anchorName == undefined){
        throw('You need to specify a constraintName when calling addAnchor!');
      }
      if(anchor == undefined){
        throw('You need to specify a anchor when calling addAnchor!');
      }
      if(rigidBodyName == undefined){
        throw('You need to specify a rigidBodyName when calling addAnchor!');
      }
      if(softBodyName == undefined){
        throw('You need to specify a softBodyName when calling addAnchor!');
      }
      // check if we are dealing with an array
      var i, isArray = anchor.constructor.toString().indexOf("Array") != -1;
      if(isArray) {
        for(i=0; i<anchor.length; i++) {
          anchor[i].name = anchorName;
        }
      }
      else {
        anchor.name = anchorName;
      }
      sbddgnode.addMember(anchorName, 'BulletAnchor[]', isArray ? anchor : [anchor]);

      // create rigid body operator
      sbddgnode.bindings.append(scene.constructOperator({
        operatorName: 'createBulletAnchor',
        parameterLayout: [
          'simulation.world',
          'self.'+anchorName,
          'rigidBodies.'+rigidBodyName+'Rbd',
          'self.'+softBodyName+'Sbd'
        ],
        entryPoint: 'createBulletAnchor',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    };

    bulletWorldNode.pub.addForce = function(forceName,force) {
      if(forceName == undefined){
        throw('You need to specify a forceName when calling addForce!');
      }
      if(force == undefined){
        throw('You need to specify a force when calling addForce!');
      }
      // check if we are dealing with an array
      var i, isArray = force.constructor.toString().indexOf("Array") != -1;
      if(isArray) {
        for(i=0; i<force.length; i++) {
          force[i].name = forceName;
        }
      }
      else {
        force.name = forceName;
      }
      dgnode.addMember(forceName+'Force', 'BulletForce[]', isArray ? force : [force]);
      bulletWorldNode.addMemberInterface(dgnode, forceName+'Force', true);

      // create rigid body operator
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'applyBulletForce',
        parameterLayout: [
          'self.world',
          'self.'+forceName+'Force'
        ],
        entryPoint: 'applyBulletForce',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    };

    // create the ground plane
    if(options.createGroundPlane) {
      // create a shape
      // create the ground rigid body
      var groundTrans = new FABRIC.RT.Xfo({tr: new FABRIC.RT.Vec3(0,-options.groundPlaneSize,0)});
      bulletWorldNode.pub.addShape('Ground', new FABRIC.RT.BulletShape.createBox(new FABRIC.RT.Vec3(options.groundPlaneSize,options.groundPlaneSize,options.groundPlaneSize)));
      bulletWorldNode.pub.addRigidBody('Ground', new FABRIC.RT.BulletRigidBody({mass: 0, transform: groundTrans}),'Ground');
      
      if(options.createGroundPlaneGrid){
        var instanceNode = scene.constructNode('Instance', {
          geometryNode: scene.constructNode('Grid', {
            size_x: options.groundPlaneSize,
            size_z: options.groundPlaneSize,
            sections_x: 20,
            sections_z: 20 }).pub,
          materialNode: scene.constructNode('FlatMaterial').pub
        });
        instanceNode.getDGNode().setDependency(rbddgnode,'rigidbodies');
      }
    }
    
    // setup raycast relevant members
    var raycastingSetup = false;
    bulletWorldNode.setupRaycasting = function() {
      if(raycastingSetup){
        return;
      }
      dgnode.addMember('raycastEnable','Boolean',false);
      bulletWorldNode.addMemberInterface(dgnode, 'raycastEnable', true);
      scene.addEventHandlingFunctions(bulletWorldNode);
      raycastingSetup = true;
    };

    // create animation operator
    if(options.connectToSceneTime) {
      dgnode.setDependency(scene.getGlobalsNode(), 'globals');
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'stepBulletWorld',
        parameterLayout: [
          'self.world',
          'self.prevTime',
          'globals.time'
        ],
        entryPoint: 'stepBulletWorld',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    } else {
      dgnode.addMember('time','Scalar',0.0);
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'stepBulletWorld',
        parameterLayout: [
          'self.world',
          'self.prevTime',
          'self.time'
        ],
        entryPoint: 'stepBulletWorld',
        srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
      }));
    }
    
    return bulletWorldNode;
  }});

FABRIC.SceneGraph.registerNodeType('BulletRigidBodyTransform', {
  briefDesc: 'The BulletRigidBodyTransform represents a bullet physics rigid body based transform.',
  detailedDesc: 'The BulletRigidBodyTransform represents a bullet physics rigid body based transform.',
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      rigidBody: undefined,
      shape: undefined,
      bulletWorldNode: undefined,
      shapeName: undefined,
      hierarchical: false,
      createBulletRaycastEventHandler: true
    });
    
    // check if we have a rigid body!
    if(!options.rigidBody) {
      throw('You need to specify a rigidBody for this constructor!');
    }
    
    // check if we have a world node
    if(!options.bulletWorldNode) {
      throw('You need to specify a bulletWorldNode for this constructor!');
    }
    if(!options.bulletWorldNode.isTypeOf('BulletWorldNode')) {
      throw('The specified bulletWorldNode is not of type \'BulletWorldNode\'.');
    }
    var rbddgnode = scene.getPrivateInterface(options.bulletWorldNode).getRbdDGNode();
    var sbddgnode = scene.getPrivateInterface(options.bulletWorldNode).getSbdDGNode();

    // check if we have a shape node
    if(!options.shapeName) {
      throw('You need to specify a shapeName for this constructor!');
    }
    
    // reuse the transform for passive rigid bodies
    if(options.rigidBody.mass == 0.0 && !options.globalXfo) {
      options.globalXfo = options.rigidBody.transform;
    }
    
    // create the transform node
    var rigidBodyTransformNode = scene.constructNode('Transform', {name: options.name, globalXfo: options.globalXfo});

    // create the rigid body on the world
    var bodyName = rigidBodyTransformNode.pub.getName();
    options.bulletWorldNode.addRigidBody(bodyName,options.rigidBody,options.shapeName);
    var bulletWorldNode = scene.getPrivateInterface(options.bulletWorldNode);

    var dgnode = rigidBodyTransformNode.getDGNode();
    dgnode.setDependency(rbddgnode,'rigidbodies');
    dgnode.setDependency(sbddgnode,'softbodies');
    
    // check if we are using multiple rigid bodies
    if(options.rigidBody.constructor.toString().indexOf("Array") != -1)
      dgnode.resize(options.rigidBody.length);
      
    // create the query transform op
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'getBulletRigidBodyTransform',
      parameterLayout: [
        'self.index',
        'rigidbodies.'+bodyName+'Rbd',
        'self.globalXfo'
      ],
      entryPoint: 'getBulletRigidBodyTransform',
      srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl'
    }));
    
    // setup raycasting to be driven by bullet
    if(options.createBulletRaycastEventHandler) {
      var raycastEventHandler;
      rigidBodyTransformNode.getRaycastEventHandler = function() {
        if(raycastEventHandler == undefined) {
          var raycastOperator = scene.constructOperator({
            operatorName: 'raycastBulletWorld',
            srcFile: 'FABRIC_ROOT/SG/KL/bullet.kl',
            entryPoint: 'raycastBulletWorld',
            parameterLayout: [
              'raycastData.ray',
              'simulation.world',
              'simulation.raycastEnable'
            ],
            async: false
          });
  
          raycastEventHandler = rigidBodyTransformNode.constructEventHandlerNode('Raycast');
          bulletWorldNode.setupRaycasting();
          raycastEventHandler.setScope('simulation', bulletWorldNode.getDGNode());
          raycastEventHandler.setSelector('simulation', raycastOperator);
        }
        return raycastEventHandler;
      };
    }

    rigidBodyTransformNode.pub.setInitialTransform = function(val) {
      return bulletWorldNode.pub.setRigidBodyInitialTransform(bodyName,val);
    };
    rigidBodyTransformNode.pub.getInitialTransform = function(val) {
      return bulletWorldNode.pub.getRigidBodyInitialTransform(bodyName,val)[0];
    };
    
    return rigidBodyTransformNode;
  }});

FABRIC.SceneGraph.registerNodeType('BulletForceManipulator', {
  briefDesc: 'The BulletForceManipulator is a basic tool introducing new forces into a BulletWorld.',
  detailedDesc: 'The BulletForceManipulator is a basic tool introducing new forces into a BulletWorld.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    enabled: 'The manipulator can be disabled, thereby allowing other manipulators to operate.',
    factor: 'The amount of force to introduce.',
    radius: 'The radius of the force to introduce.',
    useFalloff: 'If set to true, the force is multiplied by the distance to the manipulator.',
    useTorque: 'If set to true, the force will be a torque force, otherwise it is a central force.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        enabled: true,
        factor: 100,
        radius: 1.5,
        useFalloff: true,
        useTorque: true
      });

    if (!options.bulletWorldNode) {
      throw ('bulletWorldNode not specified');
    }
    if (!options.cameraNode) {
      throw ('cameraNode not specified');
    }
    var bulletWorldNode = scene.getPrivateInterface(options.bulletWorldNode);
    var cameraNode = options.cameraNode;
    bulletWorldNode.setupRaycasting();

    // introduce a new force....!
    var force = new FABRIC.RT.BulletForce({
      enable: false,
      autoDisable: true,
      factor: options.factor,
      useFalloff: options.useFalloff,
      useTorque: options.useTorque,
      radius: options.radius
    });
    bulletWorldNode.pub.addForce('Mouse', force);

    var forceManipulatorNode = scene.constructNode('SceneGraphNode', options);
    var enabled = options.enabled;
    forceManipulatorNode.pub.enable = function(){
      enabled = true;
    }
    forceManipulatorNode.pub.disable = function(){
      enabled = false;
    }
    forceManipulatorNode.pub.setFactor = function(value){
      force.factor = value;
      bulletWorldNode.pub.setMouseForce([force]);
    };
    forceManipulatorNode.pub.setRadius = function(value){
      force.radius = value;
      bulletWorldNode.pub.setMouseForce([force]);
    };
    
    var viewportNode, cameraXfo, upaxis, swaxis, hitPosition, hitDistance;
    var mouseDownScreenPos, viewportNode;
    var getCameraValues = function(evt) {
      viewportNode = evt.viewportNode;
      mouseDownScreenPos = evt.mouseScreenPos;
      viewportNode = evt.viewportNode;
      cameraXfo = evt.cameraNode.getTransformNode().getGlobalXfo();
      swaxis = cameraXfo.ori.getXaxis();
      upaxis = cameraXfo.ori.getYaxis();
      hitPosition = evt.hitData.point;
      hitDistance = evt.hitData.distance;
    }
    
    var eventListenersAdded = false;
    var mouseDownFn = function(evt) {
      if(!enabled){
        return;
      }
      if(eventListenersAdded){
        return;
      }
      if (evt.button === 0) {
        getCameraValues(evt);
        document.addEventListener('mousemove', dragForceFn, false);
        document.addEventListener('mouseup', releaseForceFn, false);
        evt.stopPropagation();
        eventListenersAdded = true;
      }
    }
    bulletWorldNode.pub.addEventListener('mousedown_geom', mouseDownFn);
    forceManipulatorNode.pub.getMouseDownFn = function() { return mouseDownFn; }

    var dragForceFn = function(evt) {
      if(!enabled || !eventListenersAdded){
        return;
      }

      var ray = viewportNode.calcRayFromMouseEvent(evt);
      var newHitPosition = ray.start.add(ray.direction.multiplyScalar(hitDistance));
      force.origin = hitPosition;
      force.direction = newHitPosition.subtract(hitPosition);
      force.enabled = true;
      bulletWorldNode.pub.setMouseForce([force]);
      hitPosition = hitPosition.add(newHitPosition.subtract(hitPosition).multiplyScalar(0.1));

      evt.stopPropagation();
      viewportNode.redraw();
    }
    var releaseForceFn = function(evt) {
      if(!eventListenersAdded){
        return;
      }
      eventListenersAdded = false;
      document.removeEventListener('mousemove', dragForceFn, false);
      document.removeEventListener('mouseup', releaseForceFn, false);
      evt.stopPropagation();
    }

    return forceManipulatorNode;
  }});

});
