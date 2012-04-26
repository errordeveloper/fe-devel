/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/Characters",
               "SG/CharacterSolvers"], function() {





FABRIC.RT.COM = function(boneId, xfoId, parameterVarIds) {
  this.boneId = boneId != undefined ? boneId : -1;
  this.xfoId = xfoId != undefined ? xfoId : -1;
  this.parameterVarIds = parameterVarIds != undefined ? parameterVarIds : [];
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('COM', {
    members: {
      boneId: 'Integer',
      xfoId: 'Integer',
      parameterVarIds: 'Integer[]'
    },
    constructor: FABRIC.RT.COM
  });
});


FABRIC.SceneGraph.CharacterSolvers.registerSolver('COMSolver', {
  constructSolver: function(options, scene) {
    
    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode());
      
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);
    var boneId = solver.generateBoneMapping( { bone: options.bone }, ['bone']);

    var comXfoId = rigNode.addVariable('Xfo');
    var comParam0VarId = rigNode.addVariable('Scalar'); // speed
    var comParam1VarId = rigNode.addVariable('Scalar'); // acceleration
    var comParam2VarId = rigNode.addVariable('Scalar'); // gradient
    var comParam3VarId = rigNode.addVariable('Scalar'); // direction
    
    var com = new FABRIC.RT.COM(boneId.bone, comXfoId, [
      comParam0VarId,
      comParam1VarId,
      comParam2VarId,
      comParam3VarId
    ]);
    skeletonNode.addMember('com', 'COM', com);
      
    rigNode.addSolverOperator({
      operatorName: 'solveCOM',
      srcCode: 'operator solveCOM( io Xfo pose[], io Bone bones[], io COM com, Size index, io PoseVariables poseVariableSlices<> ) {\n\
  pose[com.boneId] = poseVariableSlices[index].xfoValues[com.xfoId];\n\
}',
      entryPoint: 'solveCOM',
      parameterLayout: [
        'self.pose',
        'skeleton.bones',
        'skeleton.com',
        'self.index',
        'variables.poseVariables<>'
      ]
    });
    
    solver.generateTracks = function(trackSet, trackBindings){
      var storeEulerAngles = false;
      trackSet.addXfoTrack('com.xfo', new FABRIC.RT.rgb(1, 0, 0), storeEulerAngles, trackBindings, comXfoId);
      var color = FABRIC.RT.rgb(0, 1, 1);
      trackSet.addScalarTrack('com.speed', color, trackBindings, comParam0VarId);
      trackSet.addScalarTrack('com.acceleration', color, trackBindings, comParam1VarId);
      trackSet.addScalarTrack('com.gradient', color, trackBindings, comParam2VarId);
      trackSet.addScalarTrack('com.direction', color, trackBindings, comParam3VarId);
    }
    
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: 'invertCOM',
          srcCode: 'operator invertCOM( io Xfo pose[], io Bone bones[], io COM com, io PoseVariables poseVariables ) {\n\
  poseVariables.xfoValues[com.xfoId] = pose[com.boneId];\n\
}',
          entryPoint: 'invertCOM',
          parameterLayout: [
            'sourcerig.pose',
            'skeleton.bones',
            'skeleton.com',
            'self.poseVariables'
          ]
        }));
    }
    
    return solver; 
  }
});


FABRIC.RT.LocomotionFoot = function(limbId, stepTimeVarId) {
  this.limbId = limbId != undefined ? limbId : -1;
  this.stepTimeVarId = stepTimeVarId != undefined ? stepTimeVarId : -1;
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('LocomotionFoot', {
    members: {
      limbId: 'Integer',
      stepTimeVarId: 'Integer'
    },
    constructor: FABRIC.RT.LocomotionFoot
  });
});

FABRIC.SceneGraph.CharacterSolvers.registerSolver('LocomotionFeetSolver', {
  constructSolver: function(options, scene) {
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);
    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode());

    
    var limbs = skeletonNode.getData('legs');
    var locomotionFeet = [];
    for(var i=0; i<limbs.length; i++){
      var stepTimeVarId = rigNode.addVariable('Scalar');
      locomotionFeet.push(new FABRIC.RT.LocomotionFoot(i, stepTimeVarId));
    }
    skeletonNode.addMember('locomotionFeet', 'LocomotionFoot[]', locomotionFeet);
    
    solver.generateTracks = function(trackSet, trackBindings){
      for(j=0; j<limbs.length; j++){
        var color = FABRIC.RT.rgb(1, 0, 0);
        var storeEulerAngles = false;
        trackSet.addScalarTrack(solver.getName()+j+'StepTime', color, trackBindings, locomotionFeet[j].stepTimeVarId);
      }
    }
    
    return solver; 
  }
});


FABRIC.RT.LocomotionMarker = function() {
  this.time = 0.0;
//  this.stepId = 0;
  // When we activate the clip using this marker,
  // we should set all the step Ids to this set of values. 
  this.stepIds = []; 
  this.params = [];
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('LocomotionMarker', {
    members: {
      time: 'Scalar',/*
      stepId: 'Integer',*/
      stepIds: 'Integer[]',
      params: 'Scalar[]'
    },
    constructor: FABRIC.RT.LocomotionMarker
  });
});

FABRIC.RT.FootStep = function() {
  this.prevStepMidPlantTime = 0.0;
  this.liftTime = 0.0;
  this.plantTime = 0.0;
  this.midPlantTime = 0.0;
  this.strideLength = 0.0;
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('FootStep', {
    members: {
      prevStepMidPlantTime: 'Scalar',
      liftTime: 'Scalar',
      plantTime: 'Scalar',
      midPlantTime: 'Scalar',
      strideLength: 'Scalar'
    },
    constructor: FABRIC.RT.FootStep
  });
});

FABRIC.RT.FootStepTrack = function() {
  this.footSteps = [];
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('FootStepTrack', {
    members: {
      footSteps: 'FootStep[]'
    },
    constructor: FABRIC.RT.FootStepTrack
  });
});


FABRIC.SceneGraph.registerNodeType('LocomotionCharacterAnimationContainer', {
  parentNodeDesc: 'CharacterAnimationContainer',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    
    var characterAnimationContainerNode = scene.constructNode('LinearCharacterAnimationContainer', options);
    var dgnode = characterAnimationContainerNode.getDGNode();
    dgnode.addMember('markers', 'LocomotionMarker[][]');
    dgnode.addMember('footStepTracks', 'FootStepTrack[]');
    
    var paramsdgnode, debugGeometryDraw;
    characterAnimationContainerNode.pub.bindToSourceCharacterAnimationContainer = function(
      sourceCharacterAnimationContainer,
      keyframeTrackBindings,
      rigNode,
      sampleFrequency,
      footMovementThreshold
    ){
      if (!sourceCharacterAnimationContainer.isTypeOf('CharacterAnimationContainer')) {
        throw ('Incorrect type. Must be a CharacterAnimationContainer');
      }
      if (!rigNode.isTypeOf('CharacterRig')) {
        throw ('Incorrect type. Must be a CharacterRig');
      }
      
      rigNode = scene.getPrivateInterface(rigNode);
      sourceCharacterAnimationContainer = scene.getPrivateInterface(sourceCharacterAnimationContainer);
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode());
      dgnode.setDependency(sourceCharacterAnimationContainer.getDGNode(), 'sourceCharacterAnimationContainer');
      dgnode.setDependency(skeletonNode.getDGNode(), 'skeleton');
      
      paramsdgnode = characterAnimationContainerNode.constructDGNode('ParamsDGNode');
      dgnode.setDependency(paramsdgnode, 'params');
      paramsdgnode.addMember('sampleFrequency', 'Scalar', sampleFrequency);
      paramsdgnode.addMember('footMovementThreshold', 'Scalar', footMovementThreshold);
      
    //  paramsdgnode.addMember('bindings', 'KeyframeTrackBindings', keyframeTrackBindings);
      paramsdgnode.addMember('poseVariables', 'PoseVariables', rigNode.getVariables());
      
      dgnode.addMember('debugGeometry', 'DebugGeometry' );
      debugGeometryDraw = scene.constructNode('DebugGeometryDraw', {
          dgnode: dgnode,
          debugGemetryMemberName: 'debugGeometry'
      });
      
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'matchCount',
        srcCode: 'operator matchCount(in Container parentContainer, io Container selfContainer) { selfContainer.resize( parentContainer.size() ); }',
        entryPoint: 'matchCount',
        parameterLayout: [
          'sourceCharacterAnimationContainer',
          'self'
        ],
        async: false
      }));
      
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'locomotionPreProcessing',
        srcFile: 'FABRIC_ROOT/SG/KL/locomotionPreProcessing.kl',
        entryPoint: 'locomotionPreProcessing',
        parameterLayout: [
          'sourceCharacterAnimationContainer.trackSet<>',
          'sourceCharacterAnimationContainer.bindings<>',
          'params.poseVariables',
                          
          'skeleton.bones',
          'skeleton.hubs',
          'skeleton.legs',
          'skeleton.com',
          'skeleton.locomotionFeet',
          
          'params.sampleFrequency',
          'params.footMovementThreshold',
          
          'self.trackSet',
          'self.bindings',
          'self.markers',
          'self.footStepTracks',
          'self.index',
          
          'self.debugGeometry'
        ],
        async: false,
        mainThreadOnly: true
      }));
      
      characterAnimationContainerNode.pub.preProcessTracks = function(){
        dgnode.evaluate();
      }
    }
    
    var parentWriteData = characterAnimationContainerNode.writeData;
    var parentReadData = characterAnimationContainerNode.readData;
    characterAnimationContainerNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      nodeData.markers = [];
      nodeData.footStepTracks = [];
      for(var i=0; i<nodeData.numTracks; i++){
        nodeData.markers.push(dgnode.getData('markers', i));
        nodeData.footStepTracks.push(dgnode.getData('footStepTracks', i));
      }
    };
    characterAnimationContainerNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      for(var i=0; i<nodeData.numTracks; i++){
        dgnode.setData('markers', i, nodeData.markers[i]);
        dgnode.setData('footStepTracks', i, nodeData.footStepTracks[i]);
      }
    };
    
    return characterAnimationContainerNode;
  }});


FABRIC.RT.CharacterControllerParams = function() {
  this.displacement = new FABRIC.RT.Xfo();
  this.displacementDir = new FABRIC.RT.Vec3(0,0,1);
  this.trail = [];
  this.trailLength = 0;
  this.trailCircularArrayIndex = 0;
  this.balanceXfo = new FABRIC.RT.Xfo();
  this.lift = 0;
  this.gradient = 0;
  this.state = 0;
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('CharacterControllerParams', {
    members: {
      displacement: 'Xfo',
      displacementDir: 'Vec3',
      trail: 'Xfo[]',
      trailLength: 'Scalar',
      trailCircularArrayIndex: 'Integer',
      balanceXfo: 'Xfo',
      lift: 'Scalar',
      gradient: 'Scalar',
      state: 'Integer'
    },
    constructor: FABRIC.RT.CharacterControllerParams
  });
});



FABRIC.SceneGraph.registerNodeType('LocomotionCharacterController', {
  parentNodeDesc: 'CharacterController',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      maxLinearVelocity: 10.0,
      maxAngularVelocity: 0.1,
      maxLinearAcceleration: 1.0,
      maxAngularAcceleration: 1.0,
      
      trailLength: 120.0,
      numTrailSegments: 0,
      
      gravity: -25.0,
      comHeight: 15,
      circleSize: 20,
      
      enableDebugging: true
    });
    var characterControllerNode = scene.constructNode('CharacterController', options);
    var dgnode = characterControllerNode.getDGNode();
    
    var controllerparams = new FABRIC.RT.CharacterControllerParams();
    if(options.numTrailSegments > 0){
      var trailDir = options.controllerXfo.ori.rotateVector(new FABRIC.RT.Vec3(0,0,-1));
      var trail = [];
      for(var i=options.numTrailSegments; i>0; i--){
        var trailxfo = options.controllerXfo.clone();
        trailxfo.tr = trailxfo.tr.add(trailDir.multiplyScalar((i/options.numTrailSegments) * options.trailLength));
        trail.push(trailxfo);
      }
      controllerparams.trail = trail;
      controllerparams.trailLength = options.trailLength;
      controllerparams.trailCircularArrayIndex = options.numTrailSegments-1;
    }
    
    dgnode.setDependency(scene.getGlobalsNode(), 'globals');
    
    dgnode.addMember('goalLinearVelocity', 'Vec3');
    dgnode.addMember('goalOrientation', 'Quat');
    
    dgnode.addMember('maxLinearVelocity', 'Scalar', options.maxLinearVelocity);
    dgnode.addMember('maxAngularVelocity', 'Scalar', options.maxAngularVelocity);
    dgnode.addMember('maxLinearAcceleration', 'Scalar', options.maxLinearAcceleration);
    dgnode.addMember('maxAngularAcceleration', 'Scalar', options.maxAngularAcceleration);
    dgnode.addMember('linearVelocity', 'Vec3');
    dgnode.addMember('angularVelocity', 'Scalar');
    
    dgnode.addMember('comParams', 'Scalar[]');
    
    dgnode.addMember('controllerparams', 'CharacterControllerParams', controllerparams);
    dgnode.addMember('liftVec', 'Vec3');
    dgnode.addMember('gravity', 'Scalar', options.gravity);
    dgnode.addMember('comHeight', 'Scalar', options.comHeight);
    dgnode.addMember('circleSize', 'Scalar', options.circleSize);
    
    dgnode.addMember('debugGeometry1', 'DebugGeometry' );
    var debugGeometryDraw1 = scene.constructNode('DebugGeometryDraw', {
        dgnode: dgnode,
        debugGemetryMemberName: 'debugGeometry1'
    });
    dgnode.addMember('debugGeometry2', 'DebugGeometry' );
    var debugGeometryDraw2 = scene.constructNode('DebugGeometryDraw', {
        dgnode: dgnode,
        debugGemetryMemberName: 'debugGeometry2'
    });
    characterControllerNode.pub.setDrawDebuggingToggle = function(tf){
      debugGeometryDraw1.pub.setDrawToggle(tf);
      debugGeometryDraw2.pub.setDrawToggle(tf);
    }
  
    characterControllerNode.getCharacterControllerOp = function(){
      return scene.constructOperator({
        operatorName: 'evaluateCharacterController',
        srcFile: 'FABRIC_ROOT/SG/KL/characterController.kl',
        entryPoint: 'evaluateCharacterController',
        parameterLayout: [
          'self.xfo',
          'self.controllerparams',
          
          'self.goalLinearVelocity',
          'self.goalOrientation',
      
          'self.maxLinearVelocity',
          'self.maxAngularVelocity',
          'self.maxLinearAcceleration',
          'self.maxAngularAcceleration',
          
          'self.linearVelocity',
          'self.angularVelocity',
          
          'self.comParams',
          
          'self.gravity',
          'self.comHeight',
          'self.circleSize',
          
          'self.debugGeometry1',
          'self.debugGeometry2',
          
          'globals.timestep'
        ],
        preProcessorDefinitions: {
          ENABLE_DEBUGGING: options.enableDebugging ? 'true': 'false'
        }
      });
    }
    
    return characterControllerNode;
  }});



FABRIC.SceneGraph.registerNodeType('WorldSpacePlayerCharacterController', {
  parentNodeDesc: 'CharacterController',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
    });
    var characterControllerNode = scene.constructNode('LocomotionCharacterController', options);
    var dgnode = characterControllerNode.getDGNode();
    dgnode.addMember('translationControls', 'Vec2', options.translationControls);
    dgnode.addMember('orientationControls', 'Vec2', options.orientationControls);
    
    characterControllerNode.addMemberInterface(dgnode, 'translationControls', true);
    characterControllerNode.addMemberInterface(dgnode, 'orientationControls', true);
    
    dgnode.bindings.insert(scene.constructOperator({
      operatorName: 'worldSpacePlayerControls',
      srcFile: 'FABRIC_ROOT/SG/KL/characterController.kl',
      entryPoint: 'worldSpacePlayerControls',
      parameterLayout: [
        'self.translationControls',
        'self.orientationControls',
        
        'self.maxLinearVelocity',
        'self.maxAngularVelocity',
        
        'self.goalLinearVelocity',
        'self.goalOrientation'
      ],
      preProcessorDefinitions: {
        ENABLE_DEBUGGING: 'true'
      }
    }), 0);
    
    // We always append the controller op last, 
    // because it increments the character Xfo
    dgnode.bindings.append(characterControllerNode.getCharacterControllerOp());
    
    return characterControllerNode;
  }
});
  

FABRIC.SceneGraph.registerNodeType('ScreenSpacePlayerCharacterController', {
  parentNodeDesc: 'CharacterController',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      cameraNode: undefined
    });
    var cameraNode = scene.getPrivateInterface(options.cameraNode);
    var characterControllerNode = scene.constructNode('LocomotionCharacterController', options);
    var dgnode = characterControllerNode.getDGNode();
    dgnode.setDependency(cameraNode.getDGNode(), 'camera');
    dgnode.addMember('translationControls', 'Vec2', options.translationControls);
    dgnode.addMember('orientationControls', 'Vec2', options.orientationControls);
    
    characterControllerNode.addMemberInterface(dgnode, 'translationControls', true);
    characterControllerNode.addMemberInterface(dgnode, 'orientationControls', true);
    
    dgnode.bindings.insert(scene.constructOperator({
      operatorName: 'screenSpacePlayerControls',
      srcFile: 'FABRIC_ROOT/SG/KL/characterController.kl',
      entryPoint: 'screenSpacePlayerControls',
      parameterLayout: [
        'self.translationControls',
        'self.orientationControls',
        
        'self.maxLinearVelocity',
        'self.maxAngularVelocity',
        
        'self.goalLinearVelocity',
        'self.goalOrientation',
    
        'camera.cameraMat44',
        'camera.projectionMat44'
      ],
      preProcessorDefinitions: {
        ENABLE_DEBUGGING: 'true'
      }
    }), 0);
    
    // We always append the controller op last, 
    // because it increments the character Xfo
    dgnode.bindings.append(characterControllerNode.getCharacterControllerOp());
    
    return characterControllerNode;
  }
});
  
  
FABRIC.RT.TrackSetController = function( activeTrackSet ) {
  this.activeTrackSet = activeTrackSet != undefined ? activeTrackSet : 0;
  this.activateTrackSetTime = 0;
  this.time = 0.0;
  this.currKeys = [];
  this.trackWeight = 0.0;
  this.tick = 0;
  this.comParams = [];
  this.stepIds = [];
  this.deactivate = true;
  this.pivotFoot = 0;
  this.pivotStepId = 0;
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('TrackSetController', {
    members: {
      activeTrackSet: 'Integer',
      activateTrackSetTime: 'Scalar',
      time: 'Scalar',
      currKeys: 'Integer[]',
      trackWeight: 'Scalar',
      tick: 'Size',
      comParams: 'Scalar[]',
      stepIds: 'Integer[]',
      deactivate: 'Boolean',
      pivotFoot: 'Integer',
      pivotStepId: 'Integer'
    },
    constructor: FABRIC.RT.TrackSetController
  });
});

FABRIC.SceneGraph.registerNodeType('LocomotionPoseVariables', {
  parentNodeDesc: 'CharacterVariables',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      bulletWorldNode: undefined,
      enableDebugging: true,
      minFrequency: 0.5
    });
    if(!options.characterRigNode){
      throw "characterRigNode must be provided";
    }
    var characterRigNode = scene.getPrivateInterface(options.characterRigNode);
    
    options.poseVariables = characterRigNode.getVariables();
    var locomotionVariables = scene.constructNode('CharacterVariables', options);
    var dgnode = locomotionVariables.getDGNode();
    
    if(options.bulletWorldNode){
      var bulletWorldNode = scene.getPrivateInterface(options.bulletWorldNode);
      dgnode.setDependency(bulletWorldNode.getDGNode(), 'bulletworld');
    }
    dgnode.addMember('state', 'Integer');
    dgnode.addMember('trackcontroller', 'TrackSetController');
    dgnode.addMember('plantedFeet', 'Boolean[]');
    dgnode.addMember('plantLocations', 'Xfo[]');
    dgnode.addMember('hitPositions', 'Xfo[]');
    dgnode.addMember('hitPositionsPrevStep', 'Xfo[]');
    dgnode.addMember('footXfos', 'Xfo[]');
    
    
    var poseError = locomotionVariables.getVariables();
    poseError.setIdentity();
    dgnode.addMember('poseError', 'PoseVariables', poseError);
    dgnode.addMember('prevUpdatePose', 'PoseVariables', poseError);
    dgnode.addMember('clipActivationTime', 'Scalar', -1.0);
    dgnode.addMember('minFrequency', 'Scalar', options.minFrequency);
    
    dgnode.addMember('debugFootMotion', 'DebugGeometry' );
    var debugFootMotionDraw = scene.constructNode('DebugGeometryDraw', {
        dgnode: dgnode,
        debugGemetryMemberName: 'debugFootMotion'
    });
    
    dgnode.addMember('debugRaycasting', 'DebugGeometry' );
    var debugRaycastingDraw = scene.constructNode('DebugGeometryDraw', {
        dgnode: dgnode,
        debugGemetryMemberName: 'debugRaycasting'
    });
    
    dgnode.setDependency(scene.getGlobalsNode(), 'globals');
  
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'matchCount',
      srcCode: 'operator matchCount(in Container parentContainer, io Container selfContainer) { selfContainer.resize( parentContainer.size() ); }',
      entryPoint: 'matchCount',
      parameterLayout: [
        'charactercontroller',
        'self'
      ],
      async: false
    }));
    var parameterLayout = [
        'globals.timestep',
        
        'characteranimationcontainer.trackSet<>',
        'characteranimationcontainer.markers<>',
        'characteranimationcontainer.footStepTracks<>',
        'characteranimationcontainer.bindings<>',
        
        'skeleton.hubs',
        'skeleton.legs',
        'skeleton.com',
        'skeleton.locomotionFeet',
        
        'charactercontroller.xfo<>',
        'charactercontroller.controllerparams<>',
        'charactercontroller.comParams<>',
        
        'self.state',
        'self.trackcontroller',
        'self.plantedFeet',
        'self.plantLocations',
        'self.hitPositions',
        'self.hitPositionsPrevStep',
        'self.footXfos',
        
        'self.poseVariables',
        'self.prevUpdatePose',
        'self.poseError',
        'self.clipActivationTime',
        'self.minFrequency',
        'self.index',
        
        'self.debugFootMotion',
        'self.debugRaycasting'
      ];
    if(options.bulletWorldNode){
      parameterLayout.push('bulletworld.world');
    }
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'evaluateLocomotionPoseVariables',
      srcFile: 'FABRIC_ROOT/SG/KL/locomotion.kl',
      entryPoint: 'evaluateLocomotionPoseVariables',
      parameterLayout: parameterLayout,
      preProcessorDefinitions: {
        BULLLETWORLD_OPENBLOCKCOMMENT: options.bulletWorldNode ? '' : '/*',
        BULLLETWORLD_CLOSEBLOCKCOMMENT: options.bulletWorldNode ? '' : '*/',
        ENABLE_DEBUGGING: options.enableDebugging ? 'true' : 'false'
      }
    }));
    
    locomotionVariables.addReferenceInterface('CharacterController', 'LocomotionCharacterController',
      function(nodePrivate){
        dgnode.setDependency(nodePrivate.getDGNode(), 'charactercontroller');
      });
    
    locomotionVariables.addReferenceInterface('CharacterAnimationContainer', 'CharacterAnimationContainer',
      function(nodePrivate){
        dgnode.setDependency(nodePrivate.getDGNode(), 'characteranimationcontainer');
      });
    
    locomotionVariables.addReferenceInterface('Skeleton', 'CharacterSkeleton',
      function(nodePrivate){
      dgnode.setDependency(nodePrivate.getDGNode(), 'skeleton');
    });
    
    locomotionVariables.pub.setMatchCountNode = function(node){
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'matchCount',
        srcCode: 'operator matchCount(in Container parentContainer, io Container selfContainer) { selfContainer.resize( parentContainer.size() ); }',
        entryPoint: 'matchCount',
        parameterLayout: [
          'matchCountNode',
          'self'
        ],
        async: false
      }));
    }
    
    locomotionVariables.pub.setDrawFootMotionDebuggingToggle = function(tf){
      debugFootMotionDraw.pub.setDrawToggle(tf);
    }
    locomotionVariables.pub.setDrawRaycastingDebuggingToggle = function(tf){
      debugRaycastingDraw.pub.setDrawToggle(tf);
    }
    
    locomotionVariables.pub.setSkeletonNode(characterRigNode.pub.getSkeletonNode());
    locomotionVariables.pub.setCharacterControllerNode(characterRigNode.pub.getControllerNode());
    
    return locomotionVariables;
  }});


FABRIC.SceneGraph.CharacterSolvers.registerSolver('HeadLookAtSolver', {
  constructSolver: function(options, scene) {
    scene.assignDefaults(options, {
        rigNode: undefined,
        manipulatorSizes: undefined
      });
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();
    
    var headParams = options.bones;
    var boneIDs = solver.generateBoneMapping(headParams, ['bone', ['neckBones']]);
    var headXfo = referencePose[boneIDs.bone];
    var referenceAlignmentVec = headXfo.ori.inverse().rotateVector(new FABRIC.RT.Vec3(0,0,1));
    
    skeletonNode.addMember('headBoneId', 'Integer', boneIDs.bone);
    skeletonNode.addMember('neckBoneIds', 'Integer[]', boneIDs.neckBones);
    skeletonNode.addMember('referenceAlignmentVec', 'Vec3', referenceAlignmentVec);
    rigNode.addMember(name + 'alignmentVec', 'Vec3', new FABRIC.RT.Vec3(0,0,1));
    rigNode.addSolverOperator({
      operatorName: 'solveHeadLookAt',
      srcFile: 'FABRIC_ROOT/SG/KL/solveHubRig.kl',
      entryPoint: 'solveHeadLookAt',
      parameterLayout: [
        'self.pose',
        'skeleton.headBoneId',
        'skeleton.neckBoneIds',
        'skeleton.referenceAlignmentVec',
        'self.'+ name + 'alignmentVec',
        'charactercontroller.xfo<>',
        'charactercontroller.goalLinearVelocity<>',
        'self.index',
        'self.debugGeometry'
      ]
    });
    
    return solver; 
  }
});



});
