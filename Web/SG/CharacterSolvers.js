/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Characters",
                "RT/BezierKeyframe"], function() {
  
FABRIC.RT.SolverHelperFunctions = function() {
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('SolverHelperFunctions', {
    members: {
    },
    constructor: FABRIC.RT.SolverHelperFunctions,
    klBindings: {
      filename: 'solverHelperFunctions.kl',
      sourceCode: FABRIC.loadResourceURL('FABRIC_ROOT/SG/KL/solverHelperFunctions.kl')
    }
  });
});


/**
 * A character solver represents one computation in
 * the evaluation of a character's pose.
 */
FABRIC.SceneGraph.CharacterSolvers = {
  solvers: {},
  registerSolver: function(type, factoryFn) {
    if (this.solvers[type]) {
      throw ("CharacterSolver '" + type + "' already Registered");
    }else {
      this.solvers[type] = factoryFn;
    }
  },
  constructSolver: function(type, options, scene) {
    if (!this.solvers[type]) {
      throw ("CharacterSolver '" + type + "' is not registered.");
    }
    var solver = this.solvers[type].constructSolver(options, scene);
    solver.name = options.name;
    solver.type = type;
    return solver;
  }
};

FABRIC.SceneGraph.CharacterSolvers.registerSolver('CharacterSolver', {
  constructSolver: function(options, scene) {

    if (!options.rigNode) {
      throw ('You must specify the rigNode.');
    }

    var name = options.name,
      parameterLayout,
      skeletonNode = options.rigNode.getSkeletonNode(),
      manipulators = {},
      name2id;
    
    var solver = {
      getName: function(){
        return name;
      },
      constructManipulator: function(name, manipulatorType, options) {
        if (manipulators[name]) {
          throw (" Manipulator names must be unique. Solver '" + name + "' already contains :'" + name + "'");
        }
        manipulators[name] = scene.constructNode(manipulatorType, options);
        return manipulators[name];
      },
      getManipulators: function() {
        return manipulators;
      },
      generateBoneMapping: function(bones, identifiers){
        // Define the mapping between the named bones and the bone ids.
        if (!bones) {
          throw ('We need to specify the bones dictionary for any CharacterSolver.');
        }
        if (!identifiers) {
          throw ('We need to specify the identifiers for any CharacterSolver.');
        }
        if(!name2id){
          name2id = {};
          var boneNames = skeletonNode.getBoneNames();
          for (var i = 0; i < boneNames.length; i++) {
            name2id[boneNames[i]] = i;
          }
        }
    
        // create a temp map for boneName 2 id
        var boneIDs = {};
        // let's check the identifiers
        for (var i = 0; i < identifiers.length; i++){
          if (identifiers[i].constructor.name === 'Array') {
            var name = identifiers[i][0];
            if (!bones[name]) {
              console.warn ("'" + name + "' were not specified.");
              continue;
            }
            if (typeof(bones[name]) !== typeof(identifiers[i])) {
              throw ('Bone ' + name + ' was uncorrectly specified. Should be name array.');
            }
            var chainBones = [];
            var dynNames = bones[name];
            for (var j = 0; j < dynNames.length; j++) {
              if (name2id[dynNames[j]] == undefined) {
                throw ('Bone ' + dynNames[j] + ' is not part of the skeleton.');
              }
              chainBones.push(name2id[dynNames[j]]);
            }
            boneIDs[name] = chainBones;
          }
          else {
            var name = identifiers[i];
            if (!bones[name]) {
              console.warn ("'" + name + "' were not specified.");
              continue;
            }
            if (name2id[bones[name]] == undefined) {
              throw ('Bone ' + bones[name] + ' is not part of the skeleton.');
            }
            boneIDs[name] = name2id[bones[name]];
          }
        }
        return boneIDs;
      }
    };

    return solver;
  }
});


FABRIC.RT.FKHierarchy = function(boneIds, xfoIds) {
  this.boneIds = boneIds ? boneIds : [];
  this.xfoIds = xfoIds ? xfoIds : [];
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('FKHierarchy', {
    members: {
      boneIds: 'Integer[]',
      xfoIds: 'Integer[]'
    },
    constructor: FABRIC.RT.FKHierarchy
  });
});


FABRIC.SceneGraph.CharacterSolvers.registerSolver('FKHierarchySolver',{
  constructSolver: function(options, scene) {
    
    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose();

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);
    
    if(!options.bones){
      // Collect all the bones in the skeleton.
      options.bones = { bones:[] };
      for(var i=0; i<bones.length; i++){
        options.bones.bones.push(bones[i].name);
      }
    }
    
    var boneIDs = solver.generateBoneMapping(options.bones, [['bones']]),
      name = solver.getName();

    var defaultValues = [];
    for(var i=0; i<boneIDs.bones.length; i++){
      defaultValues.push(referenceLocalPose[boneIDs.bones[i]]);
    }
    var xfoIds = rigNode.addVariable('Xfo[]', defaultValues);
    
    var binding = new FABRIC.RT.FKHierarchy(boneIDs.bones, xfoIds);
    skeletonNode.addMember(name + 'bindings', 'FKHierarchy', binding);
    
    rigNode.addSolverOperator({
      operatorName: 'solveFKHierarchy',
      srcFile: 'FABRIC_ROOT/SG/KL/solveFKHierarchy.kl',
      entryPoint: 'solveFKHierarchy',
      parameterLayout: [
        'self.pose',
        'skeleton.bones',
        'skeleton.' + name + 'bindings',
        'self.index',
        'variables.poseVariables<>'
      ]
    });
    
    solver.getXfoVarBindings = function(){
      var boneXfoVarBindings = {};
      for(var i=0; i<options.bones.bones.length; i++){
        boneXfoVarBindings[options.bones.bones[i]] = xfoIds[i];
      }
      return boneXfoVarBindings;
    }
    
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: 'invertFKHierarchy',
          srcFile: 'FABRIC_ROOT/SG/KL/solveFKHierarchy.kl',
          entryPoint: 'invertFKHierarchy',
          parameterLayout: [
            'sourcerig.pose',
            'skeleton.bones',
            'skeleton.' + name + 'bindings',
            
            'self.poseVariables'
          ]
        }));
    }
    
    solver.generateTracks = function(trackSet, trackBindings){
      var color = FABRIC.RT.rgb(1, 0, 0);
      var storeEulerAngles = false;
      for (var i = 0; i < binding.boneIds.length; i++) {
        trackSet.addXfoTrack(solver.getName()+i, color, storeEulerAngles, trackBindings, xfoIds[i]);
      }
    }
 
    solver.setPose = function(pose, variablesNode) {
      var fkpose = [];
      for (var i = 0; i < binding.boneIds.length; i++) {
        if (bones[binding.boneIds[i]].parent == -1)
          fkpose[i] = pose[binding.boneIds[i]];
        else
          fkpose[i] = pose[bones[binding.boneIds[i]].parent].inverse().multiply(pose[binding.boneIds[i]]);
      }
      variablesNode.setValues(fkpose, xfoIds);
    };
    
    return solver;
  }
});




FABRIC.SceneGraph.CharacterSolvers.registerSolver('FKChainSolver',{
  constructSolver: function(options, scene) {
    
    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose();

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);
    
    var boneIDs = solver.generateBoneMapping(options.bones, [['bones']]),
      name = solver.getName();

    // check if the bones have a length
    for (i = 0; i < boneIDs.bones.length; i++) {
      if (bones[boneIDs.bones[i]].length <= 0.0) {
        throw ('Cannot apply FKChainSolver to bone ' +
          bones[boneIDs.bones[i]].name + ', because it has length == 0.');
      }
      if (i > 0 && bones[boneIDs.bones[i]].parent != boneIDs.bones[i - 1]) {
        throw ('Cannot apply FKChainSolver to bone ' +
          bones[boneIDs.bones[i]].name + ', because is not part of a single chain.');
      }
      chainLocalReferencePose.push(referenceLocalPose[boneIDs.bones[i]]);
      chainGlobalReferencePose.push(referencePose[boneIDs.bones[i]]);
    }

    var defaultValues = [];
    for(var i=0; i<boneIDs.bones.length; i++){
      defaultValues.push(referenceLocalPose[boneIDs.bones[i]]);
    }
    var xfoIds = rigNode.addVariable('Xfo[]', defaultValues);
    
    var binding = new FABRIC.RT.FKHierarchy(boneIDs.bones, xfoIds);
    skeletonNode.addMember(name + 'bindings', 'FKHierarchy', binding);
    
    rigNode.addSolverOperator({
      operatorName: 'solveFKHierarchy',
      srcFile: 'FABRIC_ROOT/SG/KL/solveFKHierarchy.kl',
      entryPoint: 'solveFKHierarchy',
      parameterLayout: [
        'self.pose',
        'skeleton.bones',
        'skeleton.' + name + 'bindings',
        'self.index',
        'variables.poseVariables<>'
      ]
    });
    
    solver.getXfoVarBindings = function(){
      var boneXfoVarBindings = {};
      for(var i=0; i<options.bones.bones.length; i++){
        boneXfoVarBindings[options.bones.bones[i]] = xfoIds[i];
      }
      return boneXfoVarBindings;
    }
    
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: 'invertFKHierarchy',
          srcFile: 'FABRIC_ROOT/SG/KL/solveFKHierarchy.kl',
          entryPoint: 'invertFKHierarchy',
          parameterLayout: [
            'sourcerig.pose',
            'skeleton.bones',
            'skeleton.' + name + 'bindings',
            
            'self.poseVariables'
          ]
        }));
    }
    
    solver.generateTracks = function(trackSet, trackBindings){
      var color = FABRIC.RT.rgb(1, 0, 0);
      var storeEulerAngles = false;
      for (var i = 0; i < binding.boneIds.length; i++) {
        trackSet.addXfoTrack(solver.getName()+i, color, storeEulerAngles, trackBindings, xfoIds[i]);
      }
    }
 
    solver.setPose = function(pose, variablesNode) {
      var fkpose = [];
      for (var i = 0; i < binding.boneIds.length; i++) {
        if (bones[binding.boneIds[i]].parent == -1)
          fkpose[i] = pose[binding.boneIds[i]];
        else
          fkpose[i] = pose[bones[binding.boneIds[i]].parent].inverse().multiply(pose[binding.boneIds[i]]);
      }
      variablesNode.setValues(fkpose, xfoIds);
    };
    
    return solver;
  }
});




FABRIC.RT.Hub = function(boneId, xfoId, hubParentSpaceId, spineBoneIds, spineBoneXfoId) {
  this.boneId = boneId != undefined ? boneId : -1;
  this.xfoId = xfoId != undefined ? xfoId : -1;
  this.hubParentSpaceId = hubParentSpaceId != undefined ? hubParentSpaceId : -1;
  this.spineBoneIds = spineBoneIds != undefined ? spineBoneIds : [];
  this.spineBoneXfoId = spineBoneXfoId != undefined ? spineBoneXfoId : [];
};

FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('Hub', {
    members: {
      boneId: 'Integer',
      xfoId: 'Integer',
      hubParentSpaceId: 'Integer',
      spineBoneIds: 'Integer[]',
      spineBoneXfoId: 'Integer[]'
    },
    constructor: FABRIC.RT.Hub
  });
});

FABRIC.SceneGraph.CharacterSolvers.registerSolver('HubSolver', {
  constructSolver: function(options, scene) {
    scene.assignDefaults(options, {
        rigNode: undefined,
        manipulatorSizes: undefined
      });
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose(),
      name = solver.getName();
    
    var hubs = [];
    var offsetXfos = [];
    for(i=0; i<options.hubs.length; i++){
      var hubParams = options.hubs[i];
      var boneIDs = solver.generateBoneMapping(hubParams, ['bone', 'parentSpaceBone', ['spineBones']]);
      var hubXfo = referencePose[boneIDs.bone];
      var spineBoneXfos = [];
      var spineBoneXfoIds;
      if(boneIDs.spineBones){
        for(var j=0; j<boneIDs.spineBones.length; j++){
          spineBoneXfos.push(bones[boneIDs.spineBones[j]].referenceLocalPose);
        }
        spineBoneXfoIds = rigNode.addVariable('Xfo[]', spineBoneXfos);
        hubXfo = referencePose[bones[boneIDs.spineBones[0]].parent].inverse().multiply(hubXfo);
      }
      var offsetXfo;
      if(hubParams.neutralXfo){
        offsetXfo = hubParams.neutralXfo.inverse().multiply(hubXfo);
      
        hubXfo = hubXfo.multiply(offsetXfo.inverse());
      }else{
        offsetXfo = new FABRIC.RT.Xfo();  
      }
      offsetXfos.push(offsetXfo);
      
      var hubXfoId = rigNode.addVariable('Xfo', hubXfo);
      var hub = new FABRIC.RT.Hub(boneIDs.bone, hubXfoId, boneIDs.parentSpaceBone, boneIDs.spineBones, spineBoneXfoIds);
      hubs.push(hub);
      if (options.createManipulators) {
        
        solver.constructManipulator(name + 'Hub'+i, 'XfoManipulator', {
          baseManipulatorType: 'CharacterManipulator',
          rigNode: rigNode.pub,
          xfoIndex: hubXfoId,
          targetName: solver.getName()+i+'Xfo',
          color: FABRIC.RT.rgb(1, 0, 0),
          size: 1,
          radius: 1,
          structIndex: i,
          attachmentOperator:{
            operatorName: 'calcHubManipulatorAttachmentXfo',
            srcFile: 'FABRIC_ROOT/SG/KL/solveHubRig.kl',
            entryPoint: 'calcHubManipulatorAttachmentXfo',
            parameterLayout: [
              'skeleton.bones',
              'skeleton.hubs',
              'skeleton.hubsoffsetXfos',
              'rig.pose',
              'charactercontroller.xfo',
              'variables.poseVariables',
              'self.structIndex',
              'self.localXfo',
              'self.parentXfo',
              'self.targetXfo',
              'self.globalXfo'
            ]
          },
          radius: options.manipulatorSizes ? options.manipulatorSizes[i] : 0.9*(1.0/(i+1)),
          screenTranslationManipulators: true,
          drawOverlaid: true
        });
      }
    }
    skeletonNode.addMember('hubs', 'Hub[]', hubs);
    skeletonNode.addMember('hubsoffsetXfos', 'Xfo[]', offsetXfos);
    rigNode.addSolverOperator({
      operatorName: 'solveHubRigs',
      srcFile: 'FABRIC_ROOT/SG/KL/solveHubRig.kl',
      entryPoint: 'solveHubRigs',
      parameterLayout: [
        'charactercontroller.xfo<>',
        'self.pose',
        'skeleton.bones',
        'skeleton.hubs',
        'skeleton.hubsoffsetXfos',
        'variables.poseVariables<>',
        'self.index',
        'self.debugGeometry'
      ]
    });
    
    solver.generateTracks = function(trackSet, trackBindings){
      for(i=0; i<hubs.length; i++){
        var color = FABRIC.RT.rgb(1, 0, 0);
        var storeEulerAngles = false;
        trackSet.addXfoTrack(solver.getName()+i+'Xfo', color, storeEulerAngles, trackBindings, hubs[i].xfoId);
      }
    }
    
    solver.setPose = function(pose, variablesNode) {
      for(var i=0; i < hubs.length; i++){
        var hub = hubs[i];
        var xfo;
        if(hub.spineBoneIds.length > 0){
          if(hub.hubParentSpaceId == -1){
          //  xfo = pose[hub.boneId];
            xfo = pose[bones[hub.spineBoneIds[0]].parent].inverse().multiply(pose[hub.boneId]);
          }
          else
            xfo = pose[hub.hubParentSpaceId].inverse().multiply(pose[hub.boneId]);
        }
        else{
          // TODO: Re-impliment hubs with parents. This made animation pre processing more cmoplex once I added the RigRoot node.
       //   if (bones[hub.boneId].parent == -1)
            xfo = pose[hub.boneId];
       //   else
       //     poseVariables.xfoValues[hub.xfoId] = pose[bones[hub.boneId].parent].inverse() * pose[hub.boneId];
        }
        variablesNode.setValue(xfo, hub.xfoId);
        if(hub.spineBoneIds.length > 0){
          var spineXfos = [];
          for (var j = 0; j < hub.spineBoneIds.size; j++) {
            if (bones[hub.spineBoneIds[j]].parent == - 1)
              spineXfos[j] = pose[hub.spineBoneIds[j]];
            else
              spineXfos[j] = pose[bones[hub.spineBoneIds[j]].parent].inverse().multiply(pose[hub.spineBoneIds[j]]);
          }
          variablesNode.setValues(spineXfos, hub.spineBoneIds);
        }
      }
    }
    ''
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
        operatorName: 'invertHubRigs',
        srcFile: 'FABRIC_ROOT/SG/KL/solveHubRig.kl',
        entryPoint: 'invertHubRigs',
        parameterLayout: [
          'sourcerig.pose',
          'skeleton.bones',
          'skeleton.hubs',
          'skeleton.hubsoffsetXfos',
          'self.poseVariables',
          'sourcerig.debugGeometry'
        ]
      }));
    }
    
    
    return solver; 
  }
});



FABRIC.RT.Limb = function(boneIds, ankleId, xfoIds, ikGoalXfoId, ikGoalOffsetXfo, ikGoalReferenceXfo, ikblendId) {
  this.boneIds = boneIds != undefined ? boneIds : [];
  this.ankleId = ankleId != undefined ? ankleId : -1;
  this.xfoIds = xfoIds != undefined ? xfoIds : [];
  this.ikGoalXfoId = ikGoalXfoId != undefined ? ikGoalXfoId : -1;
  this.ikGoalOffsetXfo = ikGoalOffsetXfo != undefined ? ikGoalOffsetXfo : new FABRIC.RT.Xfo();
  this.ikGoalReferenceXfo = ikGoalReferenceXfo != undefined ? ikGoalReferenceXfo : new FABRIC.RT.Xfo();
  this.ikblendId = ikblendId != undefined ? ikblendId : -1;
  this.hubId = -1; // This is used by the locomotion system.
};


FABRIC.appendOnCreateContextCallback(function(context) {
  context.RegisteredTypesManager.registerType('Limb', {
    members: {
      boneIds: 'Integer[]',
      ankleId: 'Integer',
      xfoIds: 'Integer[]',
      ikGoalXfoId: 'Integer',
      ikGoalOffsetXfo: 'Xfo',
      ikGoalReferenceXfo: 'Xfo',
      ikblendId: 'Integer',
      hubId: 'Integer'
    },
    constructor: FABRIC.RT.Limb
  });
});



FABRIC.SceneGraph.CharacterSolvers.registerSolver('ArmSolver', {
  constructSolver: function(options, scene) {
    scene.assignDefaults(options, {
        rigNode: undefined
      });
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose(),
      name = options.name,
      handControlXfo,
      ankleOffsetXfo,
      ankleTipXfo,
      twistManipulators = [];
      
  var addBlendInterface = function(ikblendId){
    solver['getIKBlend'+j] = function(){
      return rigNode.getValue('Number', ikblendId);
    }
    solver['setIKBlend'+j] = function(val){
      rigNode.setValue(val, ikblendId);
    }
  }
  
  var arms = [];
  for(var j=0; j<options.limbs.length; j++){
      var boneIDs = solver.generateBoneMapping(options.limbs[j], [['bones'], 'wrist']);
      
      // compute the target
      handControlXfo = referencePose[boneIDs.wrist].clone();
      handControlXfo.tr = referencePose[boneIDs.wrist].transformVector(new FABRIC.RT.Vec3(bones[boneIDs.wrist].length, 0, 0));

      wristOffsetXfo = handControlXfo.inverse().multiply(referencePose[boneIDs.wrist]);
      
      var xfos = [];
      for(var i=0; i<boneIDs.bones.length; i++){
        xfos.push(referenceLocalPose[boneIDs.bones[i]]);
      }
      xfos.push(referenceLocalPose[boneIDs.wrist]);
      var xfoIds = rigNode.addVariable('Xfo[]', xfos);
      var handControlXfoId = rigNode.addVariable('Xfo', handControlXfo);
      var ikblendId = rigNode.addVariable('Scalar', 0.0);
      
      addBlendInterface(ikblendId);
      
      var arm = new FABRIC.RT.Limb(boneIDs.bones, boneIDs.wrist, xfoIds, handControlXfoId, wristOffsetXfo, handControlXfo, ikblendId);
      arms.push(arm);
      if (options.createManipulators) {
        // add a manipulation for target and upvector
        
        solver.constructManipulator(name+j+'Hand', 'XfoManipulator', {
          baseManipulatorType: 'CharacterManipulator',
          rigNode: rigNode.pub,
          xfoIndex: handControlXfoId,
          color: FABRIC.RT.rgb(1, 0, 0),
          targetName: solver.getName()+j+'IKGoal',
          size: options.ikGoalManipulatorSizes[j],
          radius: options.ikGoalManipulatorSizes[j],
          screenTranslationManipulators: true
        });

        var xfoIndexArray = [];
        for (i = 0; i < xfoIds.length; i++) {
          xfoIndexArray.push(xfoIds[i]);
          
          if(i<xfoIds.length-1){
            solver.constructManipulator(name+j+'Bone'+i, 'BoneManipulator', {
              baseManipulatorType: 'CharacterManipulator',
              rigNode: rigNode.pub,
              xfoIndex: xfoIds[i],
              boneIndex: boneIDs.bones[0],
              extraIndexArray: xfoIndexArray,
              targetName: solver.getName()+j+'Bone'+i,
              length: bones[boneIDs.bones[i]].length,
              boneVector: new FABRIC.RT.Vec3(1, 0, 0),
              axisConstraint: ((i==2) ? new FABRIC.RT.Vec3(0, 0, 1) : undefined),
              color: FABRIC.RT.rgb(0, 0, 1),
              attachmentOperator:{
                operatorName: 'calcManipulatorChainAttachmentXfo',
                srcFile: 'FABRIC_ROOT/SG/KL/calcAttachmentXfo.kl',
                parameterLayout: [
                  'skeleton.bones',
                  'variables.poseVariables',
                  'rig.pose',
                  'self.boneIndex',
                  'self.extraIndexArray',
                  
                  'self.localXfo',
                  'self.parentXfo',
                  'self.targetXfo',
                  'self.globalXfo'
                ],
                entryPoint: 'calcManipulatorChainAttachmentXfo'
              }
            });
          }
          
          if(i>0){
          if(i<2 || i==xfoIds.length-1){
            solver.constructManipulator(name+j+'BoneOri'+i, '3AxisRotationManipulator', {
              baseManipulatorType: 'CharacterManipulator',
              rigNode: rigNode.pub,
              xfoIndex: xfoIds[i],
              boneIndex: boneIDs.bones[0],
              extraIndexArray: xfoIndexArray,
              targetName: solver.getName()+j+'Bone'+i,
              radius: options.manipulatorSizes[i],
              attachmentOperator:{
                operatorName: 'calcManipulatorChainAttachmentXfo',
                srcFile: 'FABRIC_ROOT/SG/KL/calcAttachmentXfo.kl',
                parameterLayout: [
                  'skeleton.bones',
                  'variables.poseVariables',
                  'rig.pose',
                  'self.boneIndex',
                  'self.extraIndexArray',
                  
                  'self.localXfo',
                  'self.parentXfo',
                  'self.targetXfo',
                  'self.globalXfo'
                ],
                entryPoint: 'calcManipulatorChainAttachmentXfo'
              }
            });
          }
          else{
            solver.constructManipulator(name+j+'BoneOri'+i, 'RotationManipulator', {
              baseManipulatorType: 'CharacterManipulator',
              rigNode: rigNode.pub,
              xfoIndex: xfoIds[i],
              boneIndex: boneIDs.bones[0],
              extraIndexArray: xfoIndexArray,
              targetName: solver.getName()+j+'Bone'+i,
              color: FABRIC.RT.rgb(0, 0, 0.8, 1),
              localXfo: new FABRIC.RT.Xfo({
                ori: new FABRIC.RT.Quat().setFromAxisAndAngle(new FABRIC.RT.Vec3(1, 0, 0), Math.HALF_PI)
              }),
              radius: options.manipulatorSizes[i],
              attachmentOperator:{
                operatorName: 'calcManipulatorChainAttachmentXfo',
                srcFile: 'FABRIC_ROOT/SG/KL/calcAttachmentXfo.kl',
                parameterLayout: [
                  'skeleton.bones',
                  'variables.poseVariables',
                  'rig.pose',
                  'self.boneIndex',
                  'self.extraIndexArray',
                  
                  'self.localXfo',
                  'self.parentXfo',
                  'self.targetXfo',
                  'self.globalXfo'
                ],
                entryPoint: 'calcManipulatorChainAttachmentXfo'
              }
            });
          }
          }
        }
      }
    }
    
    skeletonNode.addMember('arms', 'Limb[]', arms);
    rigNode.addSolverOperator({
      operatorName: 'solveArmRig',
      srcFile: 'FABRIC_ROOT/SG/KL/solveArmRig.kl',
      entryPoint: 'solveArmRig',
      parameterLayout: [
        'charactercontroller.xfo<>',
        'self.pose',
        'skeleton.bones',
        'skeleton.arms',
        'variables.poseVariables<>',
        'self.index',
        'self.debugGeometry'
      ]
    });
    
    solver.generateTracks = function(trackSet, trackBindings){
      for(j=0; j<arms.length; j++){
        var color = FABRIC.RT.rgb(1, 0, 0);
        var storeEulerAngles = false;
        
        trackSet.addXfoTrack(solver.getName()+j+'IKGoal', color, storeEulerAngles, trackBindings, arms[j].ikGoalXfoId);
        
        for(i=0; i<arms[j].xfoIds.length; i++){
          trackSet.addXfoTrack(solver.getName()+j+'Bone'+i, color, storeEulerAngles, trackBindings, arms[j].xfoIds[i]);
        }
        
        trackSet.addScalarTrack(solver.getName()+j+'IKBlend', color, trackBindings, arms[j].ikblendId);
      }
    }
 
 
    solver.setPose = function(pose, variablesNode) {
      for( var i=0; i<arms.length; i++){
        var arm = arms[i];
      //  var xfoValues = [];
      //  for (var j = 0; j < arm.boneIds.length; j++) {
      //    if (bones[arm.boneIds[j]].parent == - 1)
      //      xfoValues[j] = pose[arm.boneIds[j]];
      //    else
      //      xfoValues[j] = pose[bones[arm.boneIds[j]].parent].inverse().multiply(pose[arm.boneIds[j]]);
      //  }
      //  xfoValues[arm.boneIds.length] = pose[bones[arm.wristId].parent].inverse().multiply(pose[arm.wristId]);
      //  
      //  variablesNode.setValues(xfoValues, arm.xfoIds);
        
        var wristTipXfo = pose[arm.wristId];
        wristTipXfo.tr = wristTipXfo.transformVector(new FABRIC.RT.Vec3(bones[arm.wristId].length, 0.0, 0.0));
        var ikGoalXfo = wristTipXfo.multiply(arm.ikGoalOffsetXfo.inverse());
        variablesNode.setValue(ikGoalXfo, arm.ikGoalXfoId);
      }
    };
 
    
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: 'invertHumanoidLegRig',
          srcFile: 'FABRIC_ROOT/SG/KL/solveLegRig.kl',
          entryPoint: 'invertHumanoidLegRig',
          parameterLayout: [
            'sourcerig.pose',
            'skeleton.bones',
            'skeleton.arms',
            'self.poseVariables',
            'sourcerig.debugGeometry'
          ]
        }));
    }
    
    
    return solver; 
  }
});




FABRIC.SceneGraph.CharacterSolvers.registerSolver('InsectLegSolver', {
  constructSolver: function(options, scene) {
    scene.assignDefaults(options, {
        rigNode: undefined
      });
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose(),
      name = options.name,
      footControlXfo,
      lastBoneOffsetXfo,
      lastBoneTipXfo,
      i, j;
      
    var legs = [];
    for(j=0; j<options.limbs.length; j++){
      var boneIDs = solver.generateBoneMapping(options.limbs[j], [['bones']]);
      var lastBoneId = boneIDs.bones[boneIDs.bones.length-1];
      
      // compute the target
      lastBoneTipXfo = referencePose[lastBoneId].clone();
      lastBoneTipXfo.tr = referencePose[lastBoneId].transformVector(new FABRIC.RT.Vec3(bones[lastBoneId].length, 0, 0));
      footControlXfo = lastBoneTipXfo.clone();
      footControlXfo.tr.y = 0;
      var alignmentQuat = new FABRIC.RT.Quat();
      alignmentQuat.setFrom2Vectors(
        footControlXfo.ori.rotateVector(new FABRIC.RT.Vec3(1, 0, 0)),
        new FABRIC.RT.Vec3(0, -1, 0)
      );
      footControlXfo.ori = alignmentQuat.multiply(footControlXfo.ori);
      lastBoneOffsetXfo = footControlXfo.inverse().multiply(lastBoneTipXfo);
      
      var xfos = [];
      for(var i=0; i<boneIDs.bones.length; i++){
        xfos.push(referenceLocalPose[boneIDs.bones[i]]);
      }
      var xfoIds = rigNode.addVariable('Xfo[]', xfos);
      var footControlXfoId = rigNode.addVariable('Xfo', footControlXfo);
      var ikblendId = rigNode.addVariable('Scalar', 1.0);
      
      var leg = new FABRIC.RT.Limb(boneIDs.bones, -1, xfoIds, footControlXfoId, lastBoneOffsetXfo, footControlXfo, ikblendId);
      if(options.hubIds){
        leg.hubId = options.hubIds[j];
        var hubs = skeletonNode.getData('hubs');
        leg.ikGoalReferenceXfo = bones[hubs[leg.hubId].boneId].referencePose.inverse().multiply(footControlXfo);
      }
      legs.push(leg);
    }
    
    skeletonNode.addMember('legs', 'Limb[]', legs);
    rigNode.addSolverOperator({
      operatorName: 'solveInsectLegRig',
      srcFile: 'FABRIC_ROOT/SG/KL/solveLegRig.kl',
      entryPoint: 'solveInsectLegRig',
      parameterLayout: [
        'charactercontroller.xfo<>',
        'self.pose',
        'skeleton.bones',
        'skeleton.legs',
        'variables.poseVariables<>',
        'self.index',
        'self.debugGeometry'
      ]
    });
 
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
        operatorName: 'invertInsectLegRig',
        srcFile: 'FABRIC_ROOT/SG/KL/solveLegRig.kl',
        entryPoint: 'invertInsectLegRig',
        parameterLayout: [
          'sourcerig.pose',
          'skeleton.bones',
          'skeleton.legs',
          'self.poseVariables',
          'sourcerig.debugGeometry'
        ]
      }));
    }
    return solver; 
  }
});


FABRIC.SceneGraph.CharacterSolvers.registerSolver('HumanoidLegSolver', {
  constructSolver: function(options, scene) {
    scene.assignDefaults(options, {
        rigNode: undefined
      });
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose(),
      name = solver.getName(),
      footControlXfo,
      ankleOffsetXfo,
      ankleTipXfo,
      i,
      twistManipulators = [];

  var legs = [];
  for(j=0; j<options.limbs.length; j++){
      var boneIDs = solver.generateBoneMapping(options.limbs[j], [['bones'], 'ankle']);
      
      // compute the target
      ankleTipXfo = referencePose[boneIDs.ankle].clone();
      ankleTipXfo.tr = referencePose[boneIDs.ankle].transformVector(new FABRIC.RT.Vec3(bones[boneIDs.ankle].length, 0, 0));
      footControlXfo = ankleTipXfo.clone();
      footControlXfo.tr.y = 0;
      var alignmentQuat = new FABRIC.RT.Quat();
      alignmentQuat.setFrom2Vectors(
        footControlXfo.ori.rotateVector(new FABRIC.RT.Vec3(1, 0, 0)),
        new FABRIC.RT.Vec3(0, -1, 0)
      );
      footControlXfo.ori = alignmentQuat.multiply(footControlXfo.ori);
      ankleOffsetXfo = footControlXfo.inverse().multiply(ankleTipXfo);
      
      var xfos = [];
      for(var i=0; i<boneIDs.bones.length; i++){
        xfos.push(referenceLocalPose[boneIDs.bones[i]]);
      }
      xfos.push(referenceLocalPose[boneIDs.ankle]);
      var xfoIds = rigNode.addVariable('Xfo[]', xfos);
      var footControlXfoId = rigNode.addVariable('Xfo', footControlXfo);
      var ikblendId = rigNode.addVariable('Scalar', 1.0);
      
      var leg = new FABRIC.RT.Limb(boneIDs.bones, boneIDs.ankle, xfoIds, footControlXfoId, ankleOffsetXfo, footControlXfo, ikblendId);
      legs.push(leg);
      if (options.createManipulators) {
        
        solver.constructManipulator(name+j+'Foot', 'XfoManipulator', {
          baseManipulatorType: 'CharacterManipulator',
          rigNode: rigNode.pub,
          xfoIndex: footControlXfoId,
          color: FABRIC.RT.rgb(1, 0, 0),
          targetName: solver.getName()+j+'IKGoal',
          size: 1.0,
          radius: options.manipulatorSizes ? options.manipulatorSizes[options.manipulatorSizes.length-1] : 0.9,
          screenTranslationManipulators: true
        });
        
        var length = options.ikGoalManipulatorSizes ? options.ikGoalManipulatorSizes[j].x : bones[boneIDs.ankle].length * 2;
        var width = options.ikGoalManipulatorSizes ? options.ikGoalManipulatorSizes[j].y : bones[boneIDs.ankle].length;
          
        solver.constructManipulator(name+j+'FootRotate', 'PivotRotationManipulator', {
          baseManipulatorType: 'CharacterManipulator',
          rigNode: rigNode.pub,
          xfoIndex: footControlXfoId,
          targetName: name+j+'IKGoal',
          radius: length*0.5,
          geometryNode: scene.pub.constructNode('Rectangle', {
            length: length,
            width: width
          }),
          color: FABRIC.RT.rgb(0, 0, 1),
          localXfo: new FABRIC.RT.Xfo({
            ori: new FABRIC.RT.Quat().setFromAxisAndAngle(new FABRIC.RT.Vec3(0, 0, 1), Math.HALF_PI)
          })
        });
  
        /*
        solver.constructManipulator(name + 'AnkleRotate', 'BoneManipulator', {
          baseManipulatorType: 'CharacterManipulator',
          rigNode: rigNode.pub,
          targetMember: name + 'ankleIKAnimationXfo',
          parentMember: name + 'footControlXfo',
          length: bones[boneIDs.ankle].length * 2.0,
          boneVector: new FABRIC.RT.Vec3(-1, 0, 0),
          color: FABRIC.RT.rgb(0, 0, 1)
        });
        */
      }
    }
    
    skeletonNode.addMember('legs', 'Limb[]', legs);
    rigNode.addSolverOperator({
      operatorName: 'solveHumanoidLegRig',
      srcFile: 'FABRIC_ROOT/SG/KL/solveLegRig.kl',
      entryPoint: 'solveHumanoidLegRig',
      parameterLayout: [
        'charactercontroller.xfo<>',
        'self.pose',
        'skeleton.bones',
        'skeleton.legs',
        'variables.poseVariables<>',
        'self.index',
        'self.debugGeometry'
      ]
    });
    
    solver.generateTracks = function(trackSet, trackBindings){
      for(j=0; j<legs.length; j++){
        var color = FABRIC.RT.rgb(1, 0, 0);
        var storeEulerAngles = false;
        
        
        trackSet.addXfoTrack(solver.getName()+j+'IKGoal', color, storeEulerAngles, trackBindings, legs[j].ikGoalXfoId);
        /*
        for(i=0; i<legs[j].xfoIds.length; i++){
          if(legs[j].ankleId > 0 && i==legs[j].xfoIds.length-1){
            trackSet.addXfoTrack(solver.getName()+j+'Ankle', color, storeEulerAngles, trackBindings, legs[j].xfoIds[i]);
          }else{
            trackSet.addXfoTrack(solver.getName()+j+'Bone'+i, color, storeEulerAngles, trackBindings, legs[j].xfoIds[i]);
          }
        }
        */
        trackSet.addScalarTrack(solver.getName()+j+'IKBlend', color, trackBindings, legs[j].ikblendId);
      }
    }
 
 
    solver.setPose = function(pose, variablesNode) {
      for( var i=0; i<legs.length; i++){
        var leg = legs[i];
      //  var xfoValues = [];
      //  for (var j = 0; j < leg.boneIds.length; j++) {
      //    if (bones[leg.boneIds[j]].parent == - 1)
      //      xfoValues[j] = pose[leg.boneIds[j]];
      //    else
      //      xfoValues[j] = pose[bones[leg.boneIds[j]].parent].inverse().multiply(pose[leg.boneIds[j]]);
      //  }
      //  xfoValues[leg.boneIds.length] = pose[bones[leg.ankleId].parent].inverse().multiply(pose[leg.ankleId]);
      //  
      //  variablesNode.setValues(xfoValues, leg.xfoIds);
        
        var ankleTipXfo = pose[leg.ankleId];
        ankleTipXfo.tr = ankleTipXfo.transformVector(new FABRIC.RT.Vec3(bones[leg.ankleId].length, 0.0, 0.0));
        var ikGoalXfo = ankleTipXfo.multiply(leg.ikGoalOffsetXfo.inverse());
        variablesNode.setValue(ikGoalXfo, leg.ikGoalXfoId);
      }
    };
 
    
    solver.invert = function(variablesNode){
      variablesNode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: 'invertHumanoidLegRig',
          srcFile: 'FABRIC_ROOT/SG/KL/solveLegRig.kl',
          entryPoint: 'invertHumanoidLegRig',
          parameterLayout: [
            'sourcerig.pose',
            'skeleton.bones',
            'skeleton.legs',
            'self.poseVariables',
            'sourcerig.debugGeometry'
          ]
        }));
    }
    
    
    return solver; 
  }
});



FABRIC.SceneGraph.CharacterSolvers.registerSolver('DigitSolver', {
  constructSolver: function(options, scene) {

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      referenceLocalPose = skeletonNode.pub.getReferenceLocalPose(),
      name = solver.getName();
    
    var digits = [];
    var digitsxfoIds = [];
    var digitTipOffsets = [];
    for(j=0; j<options.digits.length; j++){
      var digitParams = options.digits[j];
      var boneIDs = solver.generateBoneMapping(digitParams, [['bones']]);
      
      var defaultValues = [];
      for(var i=0; i<boneIDs.bones.length; i++){
        defaultValues.push(referenceLocalPose[boneIDs.bones[i]]);
      }
      var xfoIds = rigNode.addVariable('Xfo[]', defaultValues);
      
      digitsxfoIds.push(xfoIds);
      digits.push(new FABRIC.RT.FKHierarchy(boneIDs.bones, xfoIds));
      
      var lastDigitId = boneIDs.bones[boneIDs.bones.length-1];
      var tipBoneLength = digitParams.tipBoneLength ? digitParams.tipBoneLength : bones[lastDigitId].length;
      if(digitParams.projectToGround == true){
        var digitTipXfo = referencePose[lastDigitId].clone();
        digitTipXfo.tr = digitTipXfo.tr.add(digitTipXfo.ori.getXaxis().multiplyScalar(tipBoneLength));
        digitTipXfo.tr.y = 0.0;
        digitTipOffsets.push((referencePose[lastDigitId].inverse().multiply(digitTipXfo)).tr);
      }else{
        digitTipOffsets.push(new FABRIC.RT.Vec3(tipBoneLength, 0,0));
      }
      
      
      if (options.createManipulators) {
        var xfoIndexArray = [];
        for (i = 0; i < boneIDs.bones.length; i++) {
          xfoIndexArray.push(xfoIds[i]);
          
          solver.constructManipulator(name+j+'Bone'+i, 'BoneManipulator', {
            baseManipulatorType: 'CharacterManipulator',
            rigNode: rigNode.pub,
            xfoIndex: xfoIds[i],
            boneIndex: boneIDs.bones[0],
            extraIndexArray: xfoIndexArray,
            targetName: solver.getName()+j+'Bone'+i,
            length: bones[boneIDs.bones[i]].length,
            boneVector: new FABRIC.RT.Vec3(1, 0, 0),
            axisConstraint: ((i==0) ? undefined : new FABRIC.RT.Vec3(0, 0, 1)),
            color: FABRIC.RT.rgb(0, 0, 1),
            attachmentOperator:{
              operatorName: 'calcManipulatorChainAttachmentXfo',
              srcFile: 'FABRIC_ROOT/SG/KL/calcAttachmentXfo.kl',
              parameterLayout: [
                'skeleton.bones',
                'variables.poseVariables',
                'rig.pose',
                'self.boneIndex',
                'self.extraIndexArray',
                
                'self.localXfo',
                'self.parentXfo',
                'self.targetXfo',
                'self.globalXfo'
              ],
              entryPoint: 'calcManipulatorChainAttachmentXfo'
            }
          });
        }
      }
    }
    skeletonNode.addMember(name + 'digits', 'FKHierarchy[]', digits);
    skeletonNode.addMember(name + 'digitTipOffsets', 'Vec3[]', digitTipOffsets);
    
    
 
    rigNode.addSolverOperator({
      operatorName: 'solveDigits',
      srcFile: 'FABRIC_ROOT/SG/KL/solveDigit.kl',
      entryPoint: 'solveDigits',
      parameterLayout: [
        'self.pose',
        'skeleton.bones',
        'skeleton.' + name + 'digits',
        'skeleton.' + name + 'digitTipOffsets',
        
        'self.index',
        'variables.poseVariables<>',
        
        'self.debugGeometry'
      ]
    });
      
    solver.generateTracks = function(trackSet, trackBindings){
      var color = FABRIC.RT.rgb(1, 0, 0);
      var storeEulerAngles = false;
      
      for (var i = 0; i < digits.length; i++) {
        for (var j = 0; j < digits[i].boneIds.length; j++) {
          trackSet.addXfoTrack(solver.getName()+i+''+j, color, storeEulerAngles, trackBindings, digitsxfoIds[i][j]);
        }
      }
    }
    
    return solver;
  }
});




FABRIC.SceneGraph.CharacterSolvers.registerSolver('TwistBoneSolver', {
  constructSolver: function(options, scene) {
    
    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();
      
    var twistBones = [];
    var baseBones = [];
    var targetBones = [];
    var twistWeights = [];
    var oriOffsets = [];
    for(i=0; i<options.twistBones.length; i++){
      var twistBoneParams = options.twistBones[i];
      var boneIDs = solver.generateBoneMapping(twistBoneParams, ['start', 'end', ['twistBones']]);

      // first, we will compute the local transform of the end inside the start's space
      var startXfo = referencePose[boneIDs.start];
      var endXfo = referencePose[boneIDs.end];
      
      var weights = twistBoneParams.twistWeights;
      if (!weights) {
        weights = [];
        var boneLength = startXfo.inverse().multiply(endXfo).tr.length();
        for (var j = 0; j < boneIDs.twistBones.length; j++) {
          var local = startXfo.inverse().multiply(referencePose[boneIDs.twistBones[j]]);
          var u = local.tr.length() / boneLength;
          if (u > 1.0) {
            throw ("Unexpected U value, twistBone '" + bones[boneIDs.twistBones[j]].name + "' outside of bone.");
          }
          weights.push(u);
        }
      }
      
      baseBones.push(boneIDs.start);
      targetBones.push(boneIDs.end);
      twistBones.push(boneIDs.twistBones);
      twistWeights.push(weights);
      
      var baseXfo = referencePose[boneIDs.start];
      var endXfo = referencePose[boneIDs.end];
    
      var baseVec = (endXfo.tr.subtract(baseXfo.tr)).unit();
      var endVec = endXfo.ori.getXaxis();
      var boneOffsetRotation = new FABRIC.RT.Quat();
      boneOffsetRotation.setFrom2Vectors(endVec, baseVec);
      endXfo.ori = boneOffsetRotation.multiply(endXfo.ori);
      endXfo.ori.alignWith(endXfo.ori);
      var offsets = [];
      for (var j = 0; j < boneIDs.twistBones.length; j++) {
        boneXfo = referencePose[boneIDs.twistBones[j]];
        var twistOri = baseXfo.ori.sphericalLinearInterpolate(endXfo.ori, weights[j]);
        offsets.push(twistOri.inverse().multiply(boneXfo.ori));
      }
      oriOffsets.push(offsets);
    }
    
    skeletonNode.addMember(name + 'start', 'Integer[]', baseBones);
    skeletonNode.addMember(name + 'end', 'Integer[]', targetBones);
    skeletonNode.addMember(name + 'twistBones', 'Integer[][]', twistBones);
    skeletonNode.addMember(name + 'twistWeights', 'Scalar[][]', twistWeights);
    skeletonNode.addMember(name + 'oriOffsets', 'Quat[][]', oriOffsets);

    rigNode.addSolverOperator({
        operatorName: 'solveTwistBones',
        srcFile: 'FABRIC_ROOT/SG/KL/solveTwistBones.kl',
        entryPoint: 'solveTwistBones',
        parameterLayout: [
          'self.pose',
          'skeleton.bones',
          'skeleton.' + name + 'start',
          'skeleton.' + name + 'end',
          'skeleton.' + name + 'twistBones',
          'skeleton.' + name + 'twistWeights',
          'skeleton.' + name + 'oriOffsets'
        ]
      });

    return solver;
  }
});


/*
  The Blend Bone solver is usefull for procedural bones such as knee caps.
  It interpolates 2 source bones onto a set of blend bones
*/

FABRIC.SceneGraph.CharacterSolvers.registerSolver('BlendBoneSolver', {
  constructSolver: function(options, scene) {

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();

    var blendBones = [];
    var baseBones = [];
    var targetBones = [];
    var blendWeights = [];
    var blendBoneOffsets = [];
    for(j=0; j<options.blendBones.length; j++){
      var blendBoneParams = options.blendBones[j];
      var boneIDs = solver.generateBoneMapping(blendBoneParams, ['bone', 'base', 'target']);
      
      if (!blendBoneParams.blendWeight) {
        throw ('Error in BlendBoneSolver: blendWeight not specified ');
      }
      
      var boneXfo = referencePose[boneIDs.bone];
      var baseXfo = referencePose[boneIDs.base];
      var targetXfo = referencePose[boneIDs.target];

      var blendedQuat = baseXfo.ori.sphericalLinearInterpolate(targetXfo.ori, blendBoneParams.blendWeight);
      blendBoneOffsets.push(blendedQuat.inverse().multiply(boneXfo.ori));
      
      blendBones.push(boneIDs.bone);
      baseBones.push(boneIDs.base);
      targetBones.push(boneIDs.target);
      blendWeights.push(blendBoneParams.blendWeight);
    }
    skeletonNode.addMember(name + 'blendBones', 'Integer[]', blendBones);
    skeletonNode.addMember(name + 'baseBones', 'Integer[]', baseBones);
    skeletonNode.addMember(name + 'targetBones', 'Integer[]', targetBones);
    skeletonNode.addMember(name + 'blendWeights', 'Scalar[]', blendWeights);
    skeletonNode.addMember(name + 'blendBoneOffsets', 'Quat[]', blendBoneOffsets);
    
    rigNode.addSolverOperator({
      operatorName: 'solveBlendBones',
      srcFile: 'FABRIC_ROOT/SG/KL/solveTwistBones.kl',
      entryPoint: 'solveBlendBones',
      parameterLayout: [
        'self.pose',
        'skeleton.bones',
        'skeleton.' + name + 'blendBones',
        'skeleton.' + name + 'baseBones',
        'skeleton.' + name + 'targetBones',
        'skeleton.' + name + 'blendWeights',
        'skeleton.' + name + 'blendBoneOffsets'
      ]
    });
      
    return solver;
  }
});



/*
  The Verlet Bone solver is for generating jiggle motion.
*/

FABRIC.SceneGraph.CharacterSolvers.registerSolver('VerletBoneSolver', {
  constructSolver: function(options, scene) {

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();
    
    var verletBones = [];
    var simulationWeights = [];
    var springStrengths = [];
    var dampening = [];
    var softLimitBounds = [];
    var blendWeights = [];
    var trPrev = [];
    for(j=0; j<options.verletBones.length; j++){
      var verletBoneParams = options.verletBones[j];
      var boneIDs = solver.generateBoneMapping(verletBoneParams, ['bone']);
      var boneXfo = referencePose[boneIDs.bone];
      
      verletBones.push(boneIDs.bone);
      simulationWeights.push(verletBoneParams.simulationWeight);
      springStrengths.push(verletBoneParams.springStrength);
      dampening.push(verletBoneParams.dampening);
      softLimitBounds.push(verletBoneParams.softLimitBounds);
      trPrev.push(boneXfo.tr);
    }
    skeletonNode.addMember(name + 'verletBones', 'Integer[]', verletBones);
    skeletonNode.addMember(name + 'simulationWeights', 'Scalar[]', simulationWeights);
    skeletonNode.addMember(name + 'springStrengths', 'Scalar[]', springStrengths);
    skeletonNode.addMember(name + 'dampening', 'Scalar[]', dampening);
    skeletonNode.addMember(name + 'softLimitBounds', 'Scalar[]', softLimitBounds);
    rigNode.addMember(name + 'trPrev', 'Vec3[]', trPrev);
    rigNode.addMember(name + 'Gravity', 'Vec3', options.gravity);
    rigNode.getDGNode().setDependency(scene.getGlobalsNode(), 'globals');
    
    rigNode.addSolverOperator({
      operatorName: 'solveVerletBone',
      srcFile: 'FABRIC_ROOT/SG/KL/solveVerlet.kl',
      entryPoint: 'solveVerletBone',
      parameterLayout: [
        'globals.timestep',
        'self.pose',
        'self.' + name + 'trPrev',
        'self.' + name + 'Gravity',
        'skeleton.bones',
        'skeleton.' + name + 'verletBones',
        'skeleton.' + name + 'simulationWeights',
        'skeleton.' + name + 'springStrengths',
        'skeleton.' + name + 'dampening',
        'skeleton.' + name + 'softLimitBounds',
        
        'self.debugGeometry'
      ]
    });
      
    return solver;
  }
});

FABRIC.SceneGraph.CharacterSolvers.registerSolver('VerletMuscleBoneSolver', {
  constructSolver: function(options, scene) {

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();
    
    var verletBones = [];
    var simulationWeights = [];
    var springStrengths = [];
    var dampening = [];
    var softLimitBounds = [];
    var blendWeights = [];
    var trPrev = [];
    var baseAttachmentBones = [];
    var tipAttachmentBones = [];
    var baseAttachmentOffsets = [];
    var tipAttachmentOffsets = [];
    var muscleReferenceLengths = [];
    var muscleReferenceBiases = [];
    for(j=0; j<options.muscles.length; j++){
      var muscleParams = options.muscles[j];
      var boneIDs = solver.generateBoneMapping(muscleParams, ['bone', 'baseAttachment', 'tipAttachment']);
      var boneXfo = referencePose[boneIDs.bone];
      
      verletBones.push(boneIDs.bone);
      simulationWeights.push(muscleParams.simulationWeight);
      springStrengths.push(muscleParams.springStrength);
      dampening.push(muscleParams.dampening);
      trPrev.push(boneXfo.tr);
      
      var baseAttachmentTr = boneXfo.tr.add(boneXfo.ori.getXaxis().multiplyScalar(-muscleParams.baseOffset));
      var baseAttachmentOffset = referencePose[boneIDs.baseAttachment].inverse().transformVector(baseAttachmentTr);
      
      var tipAttachmentTr = boneXfo.tr.add(boneXfo.ori.getXaxis().multiplyScalar(muscleParams.tipOffset));
      var tipAttachmentOffset = referencePose[boneIDs.tipAttachment].inverse().transformVector(tipAttachmentTr);
      
      var muscleReferenceLength = muscleParams.baseOffset + muscleParams.tipOffset;
      
      baseAttachmentBones.push(boneIDs.baseAttachment);
      tipAttachmentBones.push(boneIDs.tipAttachment);
      baseAttachmentOffsets.push(baseAttachmentOffset);
      tipAttachmentOffsets.push(tipAttachmentOffset);
      muscleReferenceLengths.push(muscleReferenceLength);
      muscleReferenceBiases.push(muscleParams.baseOffset / muscleReferenceLength);
      softLimitBounds.push(muscleParams.softLimitBounds);
    }
    
    skeletonNode.addMember(name + 'verletBones', 'Integer[]', verletBones);
    skeletonNode.addMember(name + 'simulationWeights', 'Scalar[]', simulationWeights);
    skeletonNode.addMember(name + 'springStrengths', 'Scalar[]', springStrengths);
    skeletonNode.addMember(name + 'dampening', 'Scalar[]', dampening);
    skeletonNode.addMember(name + 'softLimitBounds', 'Scalar[]', softLimitBounds);
    rigNode.addMember(name + 'trPrev', 'Vec3[]', trPrev);
    rigNode.addMember(name + 'Gravity', 'Vec3', options.gravity);
    rigNode.getDGNode().setDependency(scene.getGlobalsNode(), 'globals');
    
    skeletonNode.addMember(name + 'baseAttachmentBones', 'Integer[]', baseAttachmentBones);
    skeletonNode.addMember(name + 'baseAttachmentOffsets', 'Vec3[]', baseAttachmentOffsets);
    skeletonNode.addMember(name + 'tipAttachmentBones', 'Integer[]', tipAttachmentBones);
    skeletonNode.addMember(name + 'tipAttachmentOffsets', 'Vec3[]', tipAttachmentOffsets);
    skeletonNode.addMember(name + 'muscleReferenceLengths', 'Scalar[]', muscleReferenceLengths);
    skeletonNode.addMember(name + 'muscleReferenceBiases', 'Scalar[]', muscleReferenceBiases);
    
    
    rigNode.addSolverOperator({
      operatorName: 'solveVerletMuscleBone',
      srcFile: 'FABRIC_ROOT/SG/KL/solveVerlet.kl',
      entryPoint: 'solveVerletMuscleBone',
      parameterLayout: [
        'globals.timestep',
        'self.pose',
        'self.' + name + 'trPrev',
        'self.' + name + 'Gravity',
        'skeleton.bones',
        'skeleton.' + name + 'verletBones',
        'skeleton.' + name + 'simulationWeights',
        'skeleton.' + name + 'springStrengths',
        'skeleton.' + name + 'dampening',
        'skeleton.' + name + 'softLimitBounds',
        
        'skeleton.' + name + 'baseAttachmentBones',
        'skeleton.' + name + 'baseAttachmentOffsets',
        'skeleton.' + name + 'tipAttachmentBones',
        'skeleton.' + name + 'tipAttachmentOffsets',
        'skeleton.' + name + 'muscleReferenceLengths',
        'skeleton.' + name + 'muscleReferenceBiases',
        
        'self.debugGeometry'
      ]
    });
      
    return solver;
  }
});


FABRIC.SceneGraph.CharacterSolvers.registerSolver('VerletChainSolver', {
  constructSolver: function(options, scene) {

    var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver('CharacterSolver', options, scene);

    var rigNode = scene.getPrivateInterface(options.rigNode),
      skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode()),
      bones = skeletonNode.pub.getBones(),
      referencePose = skeletonNode.pub.getReferencePose(),
      name = solver.getName();
    
    var verletChains = [];
    var linkLengths = [];
    var referenceLengths = [];
    var simulationWeights = [];
    var springStrengths = [];
    var dampening = [];
    var blendWeights = [];
    var trPrevs = [];
    
    var baseAttachmentBones = [];
    var tipAttachmentBones = [];
    var baseAttachmentOffsets = [];
    var tipAttachmentOffsets = [];
    
    for(j=0; j<options.verletChains.length; j++){
      var verletChainParams = options.verletChains[j];
      var boneIDs = solver.generateBoneMapping(verletChainParams, [['bones'], 'baseAttachment', 'tipAttachment']);
      
      var baseXfo = referencePose[boneIDs.bones[0]];
      var baseAttachmentTr = baseXfo.tr.add(baseXfo.ori.getXaxis().multiplyScalar(-verletChainParams.baseOffset));
      var baseAttachmentOffset = referencePose[boneIDs.baseAttachment].inverse().transformVector(baseAttachmentTr);
      
      var tipXfo = referencePose[boneIDs.bones[boneIDs.bones.length-1]];
      var tipAttachmentTr = tipXfo.tr.add(tipXfo.ori.getXaxis().multiplyScalar(verletChainParams.tipOffset));
      var tipAttachmentOffset = referencePose[boneIDs.tipAttachment].inverse().transformVector(tipAttachmentTr);
      
      var chainReferenceLength = baseAttachmentTr.distanceTo(tipAttachmentTr);
      var chainLinkLengths = [verletChainParams.baseOffset];
      var trPrev = [];
      for(var i=0; i<boneIDs.bones.length; i++){
        if(i>0){
          chainLinkLengths.push(referencePose[boneIDs.bones[i]].tr.distanceTo(referencePose[boneIDs.bones[i-1]].tr));
        }
        trPrev.push(referencePose[boneIDs.bones[i]].tr);
      }
      
      verletChains.push(boneIDs.bones);
      referenceLengths.push(chainReferenceLength);
      simulationWeights.push(verletChainParams.simulationWeight);
      springStrengths.push(verletChainParams.springStrength);
      dampening.push(verletChainParams.dampening);
      
      baseAttachmentBones.push(boneIDs.baseAttachment);
      tipAttachmentBones.push(boneIDs.tipAttachment);
      baseAttachmentOffsets.push(baseAttachmentOffset);
      tipAttachmentOffsets.push(tipAttachmentOffset);
      
      chainLinkLengths.push(verletChainParams.tipOffset);
      linkLengths.push(chainLinkLengths);
      trPrevs.push(trPrev);
    }
    skeletonNode.addMember(name + 'verletChains', 'Integer[][]', verletChains);
    skeletonNode.addMember(name + 'simulationWeights', 'Scalar[]', simulationWeights);
    skeletonNode.addMember(name + 'springStrengths', 'Scalar[]', springStrengths);
    skeletonNode.addMember(name + 'dampening', 'Scalar[]', dampening);
    skeletonNode.addMember(name + 'referenceLengths', 'Scalar[]', referenceLengths);
    skeletonNode.addMember(name + 'linkLengths', 'Scalar[][]', linkLengths);
    
    rigNode.addMember(name + 'trPrev', 'Vec3[][]', trPrevs);
    rigNode.addMember(name + 'Gravity', 'Vec3', options.gravity);
    rigNode.getDGNode().setDependency(scene.getGlobalsNode(), 'globals');
    
    skeletonNode.addMember(name + 'baseAttachmentBones', 'Integer[]', baseAttachmentBones);
    skeletonNode.addMember(name + 'baseAttachmentOffsets', 'Vec3[]', baseAttachmentOffsets);
    skeletonNode.addMember(name + 'tipAttachmentBones', 'Integer[]', tipAttachmentBones);
    skeletonNode.addMember(name + 'tipAttachmentOffsets', 'Vec3[]', tipAttachmentOffsets);
    
    rigNode.addSolverOperator({
      operatorName: 'solveVerletChain',
      srcFile: 'FABRIC_ROOT/SG/KL/solveVerlet.kl',
      entryPoint: 'solveVerletChain',
      parameterLayout: [
        'globals.timestep',
        'self.pose',
        'self.' + name + 'trPrev',
        'self.' + name + 'Gravity',
        'skeleton.bones',
        
        'skeleton.' + name + 'verletChains',
        'skeleton.' + name + 'simulationWeights',
        'skeleton.' + name + 'springStrengths',
        'skeleton.' + name + 'dampening',
        
        'skeleton.' + name + 'referenceLengths',
        'skeleton.' + name + 'linkLengths',
        
        'skeleton.' + name + 'baseAttachmentBones',
        'skeleton.' + name + 'baseAttachmentOffsets',
        'skeleton.' + name + 'tipAttachmentBones',
        'skeleton.' + name + 'tipAttachmentOffsets',
        
        'self.debugGeometry'
      ]
    });
      
    return solver;
  }
});

});

