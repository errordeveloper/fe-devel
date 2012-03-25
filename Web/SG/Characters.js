/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["RT/Bone",
               "RT/PoseVariables",
               "RT/KeyframeTrackBindings",
               "SG/SceneGraph",
               "SG/Geometry",
               "SG/Animation",
               "SG/CharacterSolvers",
               "SG/DebugGeometry"], function() {


FABRIC.SceneGraph.registerNodeType('CharacterMesh', {
  briefDesc: 'The CharacterMesh node represents a deformable Triangles node.',
  detailedDesc: 'The CharacterMesh node represents a deformable Triangles node. It is typically deformed through a GLSL skinning operator, driven by a CharacterRig node.',
  parentNodeDesc: 'Triangles',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      skinningMatriciesTextureUnit: 7
    });

    var characterMeshNode = scene.constructNode('Triangles', options);
    
    characterMeshNode.pub.addVertexAttributeValue('boneIds', 'Vec4', { genVBO:true } );
    characterMeshNode.pub.addVertexAttributeValue('boneWeights', 'Vec4', { genVBO: true });
    
    characterMeshNode.pub.addUniformValue('invmatrices', 'Mat44[]');
    characterMeshNode.pub.addUniformValue('boneMapping', 'Integer[]');
    
    var redrawEventHandler = characterMeshNode.getRedrawEventHandler();
    var tex = FABRIC.RT.oglMatrixBuffer2D();
    tex.forceRefresh = true;
    redrawEventHandler.addMember('oglSkinningMatriciesTexture2D', 'OGLTexture2D', tex);
    redrawEventHandler.addMember('skinningMatriciesTextureUnit', 'Integer', options.skinningMatriciesTextureUnit);
    redrawEventHandler.addMember('textureHeight', 'Size');
    
    redrawEventHandler.preDescendBindings.append( scene.constructOperator({
      operatorName: 'loadSkinningMatricesTexture',
      srcFile: 'FABRIC_ROOT/SG/KL/loadSkinningMatrices.kl',
      preProcessorDefinitions: {
        SKINNING_MATRICES_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('skinningMatrices'),
        SKINNIMATRICIES_TEXTUREUNIT_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('skinningMatricesTextureUnit'),
        TRANSFORM_TEXTURE_HEIGHT_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('transformTextureHeight')
      },
      entryPoint: 'loadSkinningMatricesTexture',
      parameterLayout: [
        'shader.shaderProgram',
        'rig.pose<>',
        'uniforms.invmatrices',
        'uniforms.boneMapping',
        'self.oglSkinningMatriciesTexture2D',
        'self.skinningMatriciesTextureUnit',
        'self.textureHeight'
      ]
    }));
    
    characterMeshNode.pub.setInvMatrices = function(invmatrices, boneMapping) {
      characterMeshNode.getUniformsDGNode().setData('invmatrices', 0, invmatrices);
      characterMeshNode.getUniformsDGNode().setData('boneMapping', 0, boneMapping);
    };
    characterMeshNode.pub.getNumBones = function(index) {
      return characterMeshNode.getUniformsDGNode().getData('invmatrices', index).length;
    };
    
    var parentWriteData = characterMeshNode.writeData;
    var parentReadData = characterMeshNode.readData;
    characterMeshNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
    //  characterMeshNode.writeGeometryData(sceneSerializer, constructionOptions, nodeData);
      nodeData.invmatrices = characterMeshNode.getUniformsDGNode().getData('invmatrices');
      nodeData.boneMapping = characterMeshNode.getUniformsDGNode().getData('boneMapping');
    };
    characterMeshNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
    //  characterMeshNode.readGeometryData(sceneDeserializer, nodeData);
      characterMeshNode.getUniformsDGNode().setData('invmatrices', 0, nodeData.invmatrices);
      characterMeshNode.getUniformsDGNode().setData('boneMapping', 0, nodeData.boneMapping);
    };
    
    return characterMeshNode;
  }});

FABRIC.SceneGraph.registerNodeType('CharacterSkeleton', {
  briefDesc: 'The CharacterSkeleton node represents a collection of bones forming a skeleton.',
  detailedDesc: 'The CharacterSkeleton node represents a collection of bones forming a skeleton. It defines the rest pose of a character, as well as all related settings.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    calcReferenceLocalPose: 'Set to true, this will enable to computation of the local reference pose based on the global one.',
    calcReferenceGlobalPose: 'Set to true, this will enable to computation of the global reference pose based on the local one.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      calcReferenceLocalPose: false,
      calcReferenceGlobalPose: false
    });

    var characterSkeletonNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = characterSkeletonNode.constructDGNode('DGNode');
    dgnode.addMember('bones', 'Bone[]');

    // extend public interface
    characterSkeletonNode.pub.getBones = function(skeletonId) {
      return dgnode.getData('bones', skeletonId ? skeletonId : 0);
    };
    characterSkeletonNode.pub.getBone = function(boneId, skeletonId) {
      return dgnode.getData('bones', skeletonId ? skeletonId : 0)[boneId];
    };
    characterSkeletonNode.pub.getBoneIndex = function(name, skeletonId) {
      var bones = characterSkeletonNode.pub.getBones(skeletonId);
      for (var i = 0; i < bones.length; i++) {
        if (bones[i].name == name)
          return i;
      }
      return -1;
    };
    characterSkeletonNode.pub.getNumBones = function(skeletonId) {
      return characterSkeletonNode.pub.getBones().length;
    };
    characterSkeletonNode.pub.getHierarchy = function(skeletonId) {
      var result = [];
      var bones = characterSkeletonNode.pub.getBones();
      for (var i = 0; i < bones.length; i++) {
        result.push(bones[i].parent);
      }
      return result;
    };
    characterSkeletonNode.pub.getParentId = function(boneid, skeletonId) {
      return characterSkeletonNode.pub.getHierarchy(skeletonId)[boneid];
    };
    characterSkeletonNode.pub.getBoneNames = function(skeletonId) {
      var result = [];
      var bones = characterSkeletonNode.pub.getBones();
      for (var i = 0; i < bones.length; i++) {
        result.push(bones[i].name);
      }
      return result;
    };
    characterSkeletonNode.pub.getReferencePose = function(skeletonId) {
      var result = [];
      var bones = characterSkeletonNode.pub.getBones();
      for (var i = 0; i < bones.length; i++) {
        result.push(bones[i].referencePose);
      }
      return result;
    };
    characterSkeletonNode.pub.getReferenceLocalPose = function(skeletonId) {
      var result = [];
      var bones = characterSkeletonNode.pub.getBones();
      for (var i = 0; i < bones.length; i++) {
        result.push(bones[i].referenceLocalPose);
      }
      return result;
    };
    characterSkeletonNode.pub.size = function() {
      return dgnode.size();
    };
    characterSkeletonNode.pub.resize = function(count) {
      dgnode.resize(count);
    };
    characterSkeletonNode.pub.addBone = function(boneOptions, skeletonId) {
      var bones = characterSkeletonNode.pub.getBones(),
        boneMap = {};
      for (var i = 0; i < bones.length; i++) {
        boneMap[bones[i].name] = i;
      }
      
      if (boneOptions.name === undefined || boneMap[boneOptions.name]) {
        throw ('Invalid bone name specified: ' + boneOptions.name);
      }

      if (boneOptions.parent && typeof boneOptions.parent === 'string') {
        var parentIndex = boneMap[boneOptions.parent];
        if (parentIndex == -1) {
          throw ('Bone parent ' + boneOptions.parent + ' not found in skeleton.');
        }
        boneOptions.parent = parentIndex;
      }

      // compute global from local or vice versa
      if (boneOptions.referencePose && !boneOptions.referenceLocalPose) {
        if (!boneOptions.parent || boneOptions.parent == -1) {
          boneOptions.referenceLocalPose = boneOptions.referencePose;
        }
        else {
          var refPose = bones[boneOptions.parent].referencePose;
          boneOptions.referenceLocalPose = refPose.inverse().multiply(boneOptions.referencePose);
        }
      }
      else if (!boneOptions.referencePose && boneOptions.referenceLocalPose) {
        if (boneOptions.parent == -1) {
          boneOptions.referencePose = boneOptions.referenceLocalPose;
        }
        else {
          boneOptions.referencePose = boneOptions.referenceLocalPose.multiply(bones[boneOptions.parent].referencePose);
        }
      }

      var bone = new FABRIC.RT.Bone(boneOptions);
      bones.push(bone);
      dgnode.setData('bones', skeletonId ? skeletonId : 0, bones );
      return bone;
    };
    
    characterSkeletonNode.pub.setBones = function(boneOptions, skeletonId) {
      var bones = [];
      for (var i = 0; i < boneOptions.length; i++) {
        bones.push(new FABRIC.RT.Bone(boneOptions[i]));
      }
      dgnode.setData('bones', skeletonId ? skeletonId : 0, bones );
    };
    
    
    characterSkeletonNode.addMember = function(name, type, value) {
      dgnode.addMember(name, type, value);
    };
    characterSkeletonNode.getData = function(name, skeletonId) {
      return dgnode.getData(name, skeletonId ? skeletonId : 0);
    };
    characterSkeletonNode.setData = function(name, data, skeletonId) {
      dgnode.setData(name, skeletonId ? skeletonId : 0, data );
    };
    
    ////////////////////////////////////
    // Peristence
    var parentWriteData = characterSkeletonNode.writeData;
    var parentReadData = characterSkeletonNode.readData;
    characterSkeletonNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      nodeData.bones = dgnode.getData('bones');
    };
    characterSkeletonNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      dgnode.setData('bones', 0, nodeData.bones);
    };

    if (options.calcReferenceLocalPose) {
      // For skeletons that are built procedurally, or using
      // our rigging tools, this operator will run every time a
      // reference pose is modified.
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'calcReferenceLocalPose',
        srcFile: 'FABRIC_ROOT/SG/KL/characterSkeleton.kl',
        entryPoint: 'calcReferenceLocalPose',
        parameterLayout: [
          'self.bones'
        ]
      }));
    }
    if (options.calcReferenceGlobalPose) {
      // For skeletons that are built procedurally, or using
      // our rigging tools, this operator will run every time a
      // reference pose is modified.
      dgnode.bindings.append(scene.constructOperator({
        operatorName: 'calcReferenceGlobalPose',
        srcFile: 'FABRIC_ROOT/SG/KL/characterSkeleton.kl',
        entryPoint: 'calcReferenceGlobalPose',
        parameterLayout: [
          'self.bones'
        ]
      }));
    }

    return characterSkeletonNode;
  }});

FABRIC.SceneGraph.registerNodeType('CharacterSkeletonDebug', {
  briefDesc: 'The CharacterSkeletonDebug node is a debug node used to draw a CharacterSkeletonNode on screen.',
  detailedDesc: 'The CharacterSkeletonDebug node is a debug node used to draw a CharacterSkeletonNode on screen. It generates a collection of line segments for each bone in the skeleton.',
  parentNodeDesc: 'Lines',
  optionsDesc: {
    boneradius: 'The radius of the bones to use for drawing.',
    color: 'The color of the lines to use for drawing.',
    drawOverlaid: 'Set to true, this will enable overlaid drawing for the CharacterSkeletonDebug node.'
  },
  factoryFn: function(options, scene) {

    scene.assignDefaults(options, {
      boneradius: 1.0,
      color: FABRIC.RT.rgba(0.2, 0.2, 0.73, 1.0),
      drawOverlaid: true
    });

    var rigNode,
      instanceNode;

    var characterSkeletonDebug = scene.constructNode('Lines', options);

    // extend public interface
    characterSkeletonDebug.pub.setRigNode = function(node) {

      rigNode = scene.getPrivateInterface(node);
      var skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode());

      characterSkeletonDebug.getAttributesDGNode().setDependency(rigNode.getDGNode(), 'rig');
      characterSkeletonDebug.getAttributesDGNode().setDependency(skeletonNode.getDGNode(), 'skeleton');
      characterSkeletonDebug.pub.addUniformValue('boneradius', 'Scalar', options.boneradius);
      characterSkeletonDebug.pub.addUniformValue('offsetpose', 'Xfo', options.offsetpose);

      // now append the operator to create the lines
      characterSkeletonDebug.getAttributesDGNode().bindings.append(scene.constructOperator({
          operatorName: 'generateSkeletonOp',
          srcFile: 'FABRIC_ROOT/SG/KL/generateSkeleton.kl',
          entryPoint: 'generateSkeleton',
          parameterLayout: [
            'self',
            'skeleton.bones',
            'rig.pose',
            'self.positions<>',
            'uniforms.indices',
            'uniforms.boneradius',
            'uniforms.offsetpose'
          ]
        }));
    };
    characterSkeletonDebug.pub.getRigNode = function() {
      return scene.getPublicInterface(rigNode);
    };

    // make sure it deforms
    characterSkeletonDebug.pub.setAttributeDynamic('positions');

    if (options.characterRigNode) {
      characterSkeletonDebug.pub.setRigNode(options.characterRigNode);
    }

    instanceNode = scene.constructNode('Instance', {
        geometryNode: characterSkeletonDebug.pub,
        materialNode: scene.constructNode('FlatMaterial', {
          color: options.color,
          drawOverlaid: options.drawOverlaid
        }).pub
      });
    characterSkeletonDebug.pub.getInstanceNode = function() {
      return instanceNode.pub;
    }

    return characterSkeletonDebug;
  }});


FABRIC.SceneGraph.registerNodeType('CharacterVariables', {
  briefDesc: 'The CharacterVariables node is a collection of all parameters used for a pose computation.',
  detailedDesc: 'The CharacterVariables node is a collection of all parameters used for a pose computation. The members in this node are typically changing over time, for example they are based on FCurve animation.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      baseVariables: undefined,
      poseVariables: new FABRIC.RT.PoseVariables()
    });

    var characterVariablesNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = characterVariablesNode.constructDGNode('DGNode');
    dgnode.addMember('poseVariables', 'PoseVariables', options.poseVariables);
    dgnode.addMember('boundTrack', 'Integer', -1);
    
    
    // extend the private interface
    characterVariablesNode.setVariables = function(poseVariables) {
      dgnode.setData('poseVariables', 0, poseVariables);
    };
    
    characterVariablesNode.getVariables = function(){
      return dgnode.getData('poseVariables');
    };
    
    var boundToTrackAnimationContainers = false;
    var m_characterAnimationContainerNode;
    var m_trackSetId;
    var m_animationControllerNode;
    var interactiveManipulation = false;
    
    characterVariablesNode.addMemberInterface(dgnode, 'boundTrack', true, function(val){
      m_trackSetId = val;
    });
    scene.pub.addEventListener('beginmanipulation', function(evt){
      if(m_characterAnimationContainerNode){
        dgnode.setData('enableTrackEvaluation', 0, false);
        m_characterAnimationContainerNode.beginManipulation(dgnode.getData('boundTrack'));
        interactiveManipulation = true;
      }
    });
    
    scene.pub.addEventListener('endmanipulation', function(evt){
      if(m_characterAnimationContainerNode){
        m_characterAnimationContainerNode.endManipulation();
        dgnode.setData('enableTrackEvaluation', 0, true);
        interactiveManipulation = false;
      }
    });
    
    characterVariablesNode.addReferenceInterface('CharacterAnimationContainer', 'CharacterAnimationContainer',
      function(nodePrivate){
        m_characterAnimationContainerNode = nodePrivate;
        dgnode.setDependency(m_characterAnimationContainerNode.getDGNode(), 'characteranimationcontainer');
        
        if(boundToTrackAnimationContainers){
          return
        }
        dgnode.addMember('keyIndices', 'Integer[]');
        dgnode.addMember('enableTrackEvaluation', 'Boolean', true);
        
        dgnode.bindings.append(scene.constructOperator({
          operatorName: 'evaluatePoseTracks',
          srcFile: 'FABRIC_ROOT/SG/KL/evaluatePoseTracks.kl',
          preProcessorDefinitions: {
            KEYFRAMETRACKSETTYPE: m_characterAnimationContainerNode.getTrackSetType()
          },
          entryPoint: 'evaluatePoseTracks',
          parameterLayout: [
            'characteranimationcontainer.trackSet<>',
            'characteranimationcontainer.bindings<>',
            'controller.localTime',
            'self.boundTrack',
            'self.keyIndices',
            'self.poseVariables',
            'self.enableTrackEvaluation'
          ]
        }));
        boundToTrackAnimationContainers = true;
      });
    
    characterVariablesNode.addReferenceInterface('AnimationController', 'AnimationController',
      function(nodePrivate){
        m_animationControllerNode = nodePrivate;
        dgnode.setDependency(m_animationControllerNode.getDGNode(), 'controller');
      });
    
    characterVariablesNode.getValue = function(type, index) {
      var poseVariables = characterVariablesNode.getVariables();
      switch(type){
      case 'Number':
        return poseVariables.scalarValues[index];
      case 'FABRIC.RT.Vec3':
        return poseVariables.vec3Values[index];
      case 'FABRIC.RT.Quat':
        return poseVariables.quatValues[index];
      case 'FABRIC.RT.Xfo':
        return poseVariables.xfoValues[index];
      default:
        throw 'Unhandled type:' + val;
      }
    };
    
    characterVariablesNode.setValue = function(value, index, xfoTrackFilter) {
      var type = (typeof value == 'number') ? 'Number' : value.getType();
    //  if(!boundToTrackAnimationContainers){
        var poseVariables = characterVariablesNode.getVariables();
        switch(type){
        case 'Number':
          poseVariables.scalarValues[index] = value;
          break;
        case 'FABRIC.RT.Vec3':
          poseVariables.vec3Values[index] = value;
          break;
        case 'FABRIC.RT.Quat':
          poseVariables.quatValues[index] = value;
          break;
        case 'FABRIC.RT.Xfo':
          poseVariables.xfoValues[index] = value;
          break;
        default:
          throw 'Unhandled type:' + val;
        }
        characterVariablesNode.setVariables(poseVariables);
    //  }
    //  else{
      if(m_characterAnimationContainerNode){
        if(!interactiveManipulation){
          m_characterAnimationContainerNode.beginManipulation(m_trackSetId);
        }
        m_characterAnimationContainerNode.pub.setValues(m_trackSetId, m_animationControllerNode.pub.getTime(), value, index, xfoTrackFilter);
        if(!interactiveManipulation){
          m_characterAnimationContainerNode.endManipulation();
        }
      }
    }
    characterVariablesNode.setValues = function(values, indices) {
      for(var i in values){
        characterVariablesNode.setValue(values[i], indices[i]);
      }
    }
    
    //////////////////////////////////////////
    // Persistence
    var parentWriteData = characterVariablesNode.writeData;
    var parentReadData = characterVariablesNode.readData;
    characterVariablesNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
    };
    characterVariablesNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
    };
    // If a base Vaiables node is provided, we clone the values. 
    if(options.baseVariables){
      if (!options.baseVariables.isTypeOf('CharacterVariables')) {
        throw ('Incorrect type assignment. Must assign a CharacterVariables');
      }
      var baseVariables = scene.getPrivateInterface(options.baseVariables);
      poseVariables = baseVariables.getVariables();
      dgnode.setData('poseVariables', 0, poseVariables);
    }

    return characterVariablesNode;
  }});



FABRIC.SceneGraph.registerNodeType('CharacterController', {
  briefDesc: 'The CharacterController places a character in a scene.',
  detailedDesc: 'The CharacterController node is used to place a character in a scene. The animation of the character is relative to the controller. ' +
                'The Character controlle rprovides a base transfrom which the pose of the character built on.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    controllerXfo: 'The initial transfrom of the character.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      controllerXfo: new FABRIC.RT.Xfo()
    });
    var characterControllerNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = characterControllerNode.constructDGNode('DGNode');
    
    dgnode.addMember('xfo', 'Xfo', options.controllerXfo);
    
    return characterControllerNode;
  }});


// The character rig computes the pose of a character
FABRIC.SceneGraph.registerNodeType('CharacterRig', {
  briefDesc: 'The CharacterRig node is used to compute the pose of a character.',
  detailedDesc: 'The CharacterRig node is used to compute the pose of a character. It utilizes the CharacterSkeleton as well as the CharacterVariables node, ' +
                'together with a series of operators, called CharacterSolvers, to compute the global pose of the character.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    skeletonNode: 'The skeletonNode to use for this CharacterRig',
    variablesNode: 'The variablesNode to use for this CharacterRig',
    baseCharacterRigNode: 'Copy all solvers from this base rig to male a clone'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      skeletonNode: undefined,
      variablesNode: undefined,
      controllerNode: undefined,
      baseCharacterRigNode: undefined,
      debug: true
    });
    
    var characterRigNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = characterRigNode.constructDGNode('DGNode');
    var skeletonNode,
      variablesNode,
      controllerNode,
      poseVariables = new FABRIC.RT.PoseVariables(),
      solvers = [];
      
    dgnode.addMember('pose', 'Xfo[]');
    
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
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'initPose',
      srcCode: 'operator initPose(io Xfo pose[], io Bone bones[]) { \n\
        if(pose.size == 0){\n\
          pose.resize(bones.size);\n\
          for(Integer i=0; i<bones.size; i++){\n\
            pose[i] = bones[i].referencePose;\n\
          }\n\
        }\n\
      }',
      entryPoint: 'initPose',
      parameterLayout: [
        'self.pose',
        'skeleton.bones'
      ],
      async: false
    }));
    
    //////////////////////////////////////////
    // Debug Geometries
    // Debugging enables solvers to draw lines on screen to illustrate behavior.
    dgnode.addMember('debugGeometry', 'DebugGeometry', new FABRIC.RT.DebugGeometry(options.debug) );
    
    var debugGeometryDraw = scene.constructNode('DebugGeometryDraw', {
      dgnode: dgnode,
      debugGemetryMemberName: 'debugGeometry'
    });
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'resetDebugDrawing',
      srcCode: 'use DebugGeometry; operator resetDebugDrawing(io DebugGeometry debugGeometry) { debugGeometry.reset(); }',
      entryPoint: 'resetDebugDrawing',
      parameterLayout: [
        'self.debugGeometry'
      ],
      async: false
    }));
    characterRigNode.pub.getDebuggingEnabled = function(tf){
      return dgnode.getData('debugGeometry').enabled;
    }
    characterRigNode.pub.setDebuggingEnabled = function(tf){
      debugGeometryDraw.pub.setDrawToggle(tf);
      var debugGeometry = dgnode.getData('debugGeometry');
      debugGeometry.enabled = tf;
      dgnode.setData('debugGeometry', 0, debugGeometry);
    }
    
    //////////////////////////////////////////
    
    characterRigNode.addReferenceInterface('Skeleton', 'CharacterSkeleton',
      function(nodePrivate){
        skeletonNode = nodePrivate;
        dgnode.setDependency(skeletonNode.getDGNode(), 'skeleton');
  
        // This member will store the computed pose.
        var referencePose = skeletonNode.pub.getReferencePose();
        dgnode.setData('pose', 0, referencePose);
      });

    characterRigNode.addReferenceInterface('Controller', 'CharacterController',
      function(nodePrivate){
        controllerNode = nodePrivate;
        dgnode.setDependency(controllerNode.getDGNode(), 'charactercontroller');
      });
    
    //////////////////////////////////////////
    // Variables
    characterRigNode.addReferenceInterface('Variables', 'CharacterVariables',
      function(nodePrivate){
        variablesNode = nodePrivate;
        dgnode.setDependency(variablesNode.getDGNode(), 'variables');
      });
    characterRigNode.addVariable = function(type, value) {
      var id = poseVariables.addVariable(type, value);
      if(variablesNode){
        variablesNode.setVariables(poseVariables);
      }
      return id;
    };
    characterRigNode.getVariables = function(){
      return poseVariables;
    };
    characterRigNode.pub.constructVariablesNode = function(name, bindToRig) {
      var node = scene.constructNode('CharacterVariables', {
        name: name,
        poseVariables: poseVariables
      }).pub;
      if(bindToRig){
        characterRigNode.pub.setVariablesNode(node);
      }
      return node;
    };
    
    characterRigNode.getValue = function(type, index) {
      if(variablesNode){
        return variablesNode.getValue(type, index);
      }
    }
    characterRigNode.setValue = function(value, index) {
      if(variablesNode){
        variablesNode.setValue(value, index);
      }
    }
    
    //////////////////////////////////////////
    // Solver Interfaces
    var solverParams = [];
    characterRigNode.pub.addSolver = function(name, type, solverOptions) {
      for (var i = 0; i < solvers.length; i++) {
        if (solvers[i].name == name) {
          throw (" Solver names must be unique. Solver '" + name +
                 "' already applied to :" + characterRigNode.pub.getName());
        }
      }
      solverOptions = solverOptions ? solverOptions : {};
      solverOptions.rigNode = characterRigNode.pub;
      solverOptions.name = name;
      var solver = FABRIC.SceneGraph.CharacterSolvers.constructSolver(type, solverOptions, scene);
      solvers.push(solver);
      delete solverOptions.rigNode;
      solverParams.push({
        name: name,
        type: type,
        options: solverOptions
      });
      return solver;
    };
    characterRigNode.pub.getSolver = function(name) {
      for (var i = 0; i < solvers.length; i++) {
        if(solvers[i].getName() == name)
          return solvers[i];
      }
      return undefined;
    };
    
    characterRigNode.pub.getSolvers = function() {
      return solvers;
    };
    characterRigNode.addMember = function(name, type, value) {
      dgnode.addMember(name, type, value);
    };
    characterRigNode.addSolverOperator = function(operatorDef) {
      operatorBinding = scene.constructOperator(operatorDef);
      dgnode.bindings.append(operatorBinding);
    }
    characterRigNode.pub.invertSolvers = function(node, bindToRig) {
      if (!node.isTypeOf('CharacterRig')) {
        throw ('Incorrect type assignment. Must assign a CharacterRig');
      }
      sourceRigNode = scene.getPrivateInterface(node);
      var invertedVariablesNode = characterRigNode.pub.constructVariablesNode('invertedVariables', bindToRig);
      invertedVariablesNode = scene.getPrivateInterface(invertedVariablesNode);
      invertedVariablesNode.getDGNode().setDependency(sourceRigNode.getDGNode(), 'sourcerig');
      invertedVariablesNode.getDGNode().setDependency(skeletonNode.getDGNode(), 'skeleton');
      for (var i = 0; i < solvers.length; i++) {
        if(!solvers[i].invert)
          continue;
        solvers[i].invert(invertedVariablesNode);
      }
      return invertedVariablesNode.pub;
    };
    
    var m_pose;
    characterRigNode.pub.getPose = function() {
      if(solvers.length > 0){
        return dgnode.getData('pose');
      }else{
        if(!m_pose)
          m_pose = dgnode.getData('pose');
        return m_pose;
      }
    };
    characterRigNode.pub.setPose = function(pose) {
      if(solvers.length > 0){
        for (var i = 0; i < solvers.length; i++) {
          if(!solvers[i].setPose)
            continue;
          solvers[i].setPose(pose, variablesNode);
        }
      }else{
        m_pose = pose;
        dgnode.setData('pose', 0, pose);
      }
    };
    
    //////////////////////////////////////////
    // Animation Tracks
    characterRigNode.pub.generateAnimationTracks = function(node, trackSetName, bindForPlayback, controllerNode){
      if (!node.isTypeOf('CharacterAnimationContainer')) {
        throw ('Incorrect type assignment. Must assign a CharacterAnimationContainer');
      }
      var characterAnimationContainer = scene.getPrivateInterface(node);
      
      var trackBindings = new FABRIC.RT.KeyframeTrackBindings();
      var trackSet = characterAnimationContainer.newTrackSet(trackSetName);
      for (var i = 0; i < solvers.length; i++) {
        if(solvers[i].generateTracks){
          solvers[i].generateTracks(trackSet, trackBindings);
        }
      }
      var trackSetId = characterAnimationContainer.pub.addTrackSet(trackSet, trackBindings);
      if(bindForPlayback==true){
        
        if(!poseVariables){
          characterRigNode.constructVariablesNode('Variables', true);
        }
        trackSet.setPoseValues(0, poseVariables, trackBindings);
        
        if(!controllerNode){
          controllerNode = scene.pub.constructNode('AnimationController');
        }
        variablesNode.pub.setCharacterAnimationContainerNode(node);
        variablesNode.pub.setAnimationControllerNode(controllerNode);
        variablesNode.pub.setBoundTrack(trackSetId);        
      }
      return trackSetId;
    }
    characterRigNode.pub.setBoundTrackSet = function(boundTrack){
      if(!poseVariables){
        throw "animation library not configured yet.";
      }
      variablesNode.pub.setBoundTrack(boundTrack);
    }
    
    //////////////////////////////////////////
    // Persistence
    var parentAddDependencies = characterRigNode.addDependencies;
    characterRigNode.addDependencies = function(sceneSerializer) {
      parentAddDependencies(sceneSerializer);
      sceneSerializer.addNode(skeletonNode.pub);
      sceneSerializer.addNode(variablesNode.pub);
    };
    
    var parentWriteData = characterRigNode.writeData;
    var parentReadData = characterRigNode.readData;
    characterRigNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      constructionOptions.debug = options.debug;
      nodeData.skeletonNode = skeletonNode.pub.getName();
      nodeData.variablesNode = variablesNode.pub.getName();
      nodeData.solvers = [];
      for (var i = 0; i < solverParams.length; i++) {
        nodeData.solvers[i] = solverParams[i];
      }
    };
    characterRigNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      characterRigNode.pub.setSkeletonNode(sceneDeserializer.getNode(nodeData.skeletonNode));
      characterRigNode.pub.setVariablesNode(sceneDeserializer.getNode(nodeData.variablesNode));
      for (var i = 0; i < nodeData.solvers.length; i++) {
        characterRigNode.pub.addSolver(nodeData.solvers[i].name, nodeData.solvers[i].type, nodeData.solvers[i].options);
      }
    };
    
    if (options.skeletonNode) {
      characterRigNode.pub.setSkeletonNode(options.skeletonNode);
    }
    
    if (options.variablesNode) {
      characterRigNode.pub.setVariablesNode(options.variablesNode);
    }
    
    if (options.controllerNode) {
      characterRigNode.pub.setControllerNode(options.controllerNode);
    }
    else {
      characterRigNode.pub.setControllerNode(scene.constructNode('CharacterController').pub);
    }
    
    if (options.baseCharacterRigNode) {
      // Apply the baseCharacterRigNode solvers.
      var baseCharacterRigNode = scene.getPrivateInterface(options.baseCharacterRigNode);
      characterRigNode.pub.setSkeletonNode(baseCharacterRigNode.pub.getSkeletonNode());
      var baseRigSolvers = baseCharacterRigNode.getSolverParams();
      for (var i = 0; i < baseRigSolvers.length; i++) {
        characterRigNode.pub.addSolver(baseRigSolvers[i].name, baseRigSolvers[i].type, baseRigSolvers[i].options);
      }
    }
    
    return characterRigNode;
  }});



// The character instance draws a deformed mesh on screen.
FABRIC.SceneGraph.registerNodeType('CharacterInstance', {
  briefDesc: 'The CharacterInstance node is used to draw a deformed CharacterMesh on screen.',
  detailedDesc: 'The CharacterInstance node is used to draw a deformed CharacterMesh on screen. It utilizes the CharacterRig as well as the CharacterMesh, and applies skinning / deformation through the use of GLSL.',
  parentNodeDesc: 'Instance',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      });

    var characterInstanceNode = scene.constructNode('Instance', options);
    var rigNode;

    // extend public interface
    characterInstanceNode.addReferenceInterface('Rig', 'CharacterRig',
      function(nodePrivate){
        rigNode = nodePrivate;
        characterInstanceNode.getRedrawEventHandler().setScope('rig', rigNode.getDGNode());
      });
    var parentSetGeometryNode = characterInstanceNode.pub.setGeometryNode;
    characterInstanceNode.addReferenceInterface('Geometry', 'CharacterMesh',
      function(nodePrivate){
        parentSetGeometryNode(nodePrivate.pub);
      });
    
    //////////////////////////////////////////
    // Persistence
    var parentWriteData = characterInstanceNode.writeData;
    var parentReadData = characterInstanceNode.readData;
    characterInstanceNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      sceneSerializer.addNode(rigNode.pub);
      nodeData.rigNode = rigNode.pub.getName();
    };
    characterInstanceNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      characterInstanceNode.pub.setRigNode(sceneDeserializer.getNode(nodeData.rigNode));
    };

    if (options.rigNode) {
      characterInstanceNode.pub.setRigNode(options.rigNode);
    }
    return characterInstanceNode;
  }});


// The character instance draws a deformed mesh on screen.
FABRIC.SceneGraph.registerNodeType('Attachment', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: 'Transform',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      rigNode: undefined,
      attachmentBone: "boneName",
      maintainInitialXfo: true
    });

    options.hierarchical = false;
    var attachmentNode = scene.constructNode('Transform', options);
    var dgnode = attachmentNode.getDGNode();
    var boneIndex = options.boneIndex;
    
    dgnode.addMember('localXfo', 'Xfo', options.localXfo);
    dgnode.addMember('boneIndex', 'Size', options.boneIndex);
    dgnode.bindings.append(scene.constructOperator( {
        operatorName: 'calcIndexedGlobalXfo',
        srcFile: 'FABRIC_ROOT/SG/KL/calcGlobalXfo.kl',
        parameterLayout: [
          'self.localXfo',
          'characterrig.pose',
          'self.boneIndex',
          'self.globalXfo'
        ],
        entryPoint: 'calcIndexedGlobalXfo'
      }));

    attachmentNode.addMemberInterface(dgnode, 'localXfo', true, true);

    // use a custom getter
    attachmentNode.pub.setGlobalXfo = function(val) {
      if (parentTransformNode) {
        var parentXfo = parentTransformNode.getGlobalXfo();
        val = val.multiply(parentXfo.inverse());
        dgnode.setData('localXfo', val);
      }
      else {
        dgnode.setData('globalXfo', val);
      }
    };
    
    var boneIndex;
    attachmentNode.pub.setRigNode = function(node) {
      if (!node.isTypeOf('CharacterRig')) {
        throw ('Incorrect type assignment. Must assign a CharacterRig');
      }
      rigNode = scene.getPrivateInterface(node);
      dgnode.setDependency(rigNode.getDGNode(), 'characterrig');
      boneIndex = rigNode.pub.getSkeletonNode().getBoneIndex(options.attachmentBone);
      if(boneIndex == -1){
        throw("Bone not found:"+options.attachmentBone);
      }
      dgnode.setData('boneIndex', 0, boneIndex);
      if(options.maintainInitialXfo){
        var pose = rigNode.pub.getPose();
        // by setting the offset to the inverse of the bone global pose,
        // the attachment will stay in place when bound.
        dgnode.setData('localXfo', 0, pose[boneIndex].inverse());
      }
    };
    
    attachmentNode.pub.getAttachmentBoneXfo = function() {
      return rigNode.pub.getPose()[boneIndex];
    }
      
    if(options.rigNode){
      attachmentNode.pub.setRigNode(options.rigNode);
    }
    return attachmentNode;
  }});


 
// The character instance draws a deformed mesh on screen.
FABRIC.SceneGraph.registerNodeType('ManipulatorAttachment', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: 'Transform',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      rigNode: undefined,
      attachmentBone: undefined,
      xfoIndex: -1
    });

    options.hierarchical = false;
    var attachmentNode = scene.constructNode('Transform', options);
    var dgnode = attachmentNode.getDGNode();
    var xfoIndex = options.xfoIndex;
    var variablesNode;
    
    if(options.xfoIndex == -1)
      throw "xfo Index not specified";
    
    dgnode.addMember('xfoIndex', 'Size', options.xfoIndex);
    dgnode.addMember('boneIndex', 'Size', options.boneIndex);
    dgnode.addMember('structIndex', 'Size', options.structIndex);
    dgnode.addMember('extraIndex', 'Size', options.extraIndex);
    dgnode.addMember('extraIndexArray', 'Size[]', options.extraIndexArray);
    dgnode.addMember('localXfo', 'Xfo', options.localXfo);
    dgnode.addMember('targetXfo', 'Xfo');
    dgnode.addMember('parentXfo', 'Xfo');
    
    attachmentNode.getTargetXfo = function(){ return dgnode.getData('targetXfo'); }
    attachmentNode.getParentXfo = function(){ return dgnode.getData('parentXfo'); }
    
    dgnode.bindings.append(scene.constructOperator(
      options.attachmentOperator ? options.attachmentOperator : {
        operatorName: 'calcManipulatorAttachmentXfo',
        srcFile: 'FABRIC_ROOT/SG/KL/calcAttachmentXfo.kl',
        parameterLayout: [
          'charactercontroller.xfo',
          'variables.poseVariables',
          'self.xfoIndex',
          
          'self.localXfo',
          'self.parentXfo',
          'self.targetXfo',
          'self.globalXfo'
        ],
        entryPoint: 'calcManipulatorAttachmentXfo'
      }));

    attachmentNode.pub.setRigNode = function(node, boneName) {
      if (!node.isTypeOf('CharacterRig')) {
        throw ('Incorrect type assignment. Must assign a CharacterRig');
      }
      var rigNode = scene.getPrivateInterface(node);
      dgnode.setDependency(rigNode.getDGNode(), 'rig');
      var variablesNode = scene.getPrivateInterface(rigNode.pub.getVariablesNode());
      dgnode.setDependency(variablesNode.getDGNode(), 'variables');
      var skeletonNode = scene.getPrivateInterface(rigNode.pub.getSkeletonNode());
      dgnode.setDependency(skeletonNode.getDGNode(), 'skeleton');
      var controllerNode = scene.getPrivateInterface(rigNode.pub.getControllerNode());
      dgnode.setDependency(controllerNode.getDGNode(), 'charactercontroller');
    };
    if(options.rigNode){
       attachmentNode.pub.setRigNode(options.rigNode);
    }
    
    return attachmentNode;
  }});


FABRIC.SceneGraph.registerNodeType('CharacterManipulator', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: 'Manipulator',
  optionsDesc: {
  },
  manipulating: false,
  factoryFn: function(options, scene) {
    
    scene.assignDefaults(options, {
      rigNode: undefined,
      attachmentBone: "boneName",
      xfoIndex: -1,
      localXfo: new FABRIC.RT.Xfo(),
      targetName: ""
    });
    if(!options.rigNode){
      throw "Rig Node not specified";
    }
    
    // get the target node (the one we are manipulating)
    var rigNode = options.rigNode;
    var xfoIndex = options.xfoIndex;
    var xfoTrackFilter = options.xfoFilter;

    var transformNode = scene.constructNode('ManipulatorAttachment', {
      name: options.name + 'ManipulatorAttachment',
      rigNode: options.rigNode,
      
      xfoIndex: options.xfoIndex,
      boneIndex: options.boneIndex,
      structIndex: options.structIndex,
      extraIndex: options.extraIndex,
      extraIndexArray: options.extraIndexArray,
      localXfo: options.localXfo,
      attachmentOperator: options.attachmentOperator,
    
      maintainInitialXfo: false
    });
    
    options.transformNode = transformNode.pub;
    options.raycastOverlaid = options.drawOverlaid;
    var manipulatorNode = scene.constructNode('Manipulator', options );
    var xfoIndex = options.xfoIndex;
    var variablesNode;

    manipulatorNode.pub.setRigNode = function(node, boneName) {
      if (!node.isTypeOf('CharacterRig')) {
        throw ('Incorrect type assignment. Must assign a CharacterRig');
      }
      var rigNode = scene.getPrivateInterface(node);
      variablesNode = scene.getPrivateInterface(rigNode.pub.getVariablesNode());
    };
    
    
    manipulatorNode.pub.getVariableXfo = function(val) {
      return transformNode.getTargetXfo();
    };
    manipulatorNode.pub.setVariableXfo = function(val) {
      var parentXfo = transformNode.getParentXfo();
      val = parentXfo.inverse().multiply(val);
      variablesNode.setValue(val, xfoIndex, xfoTrackFilter);
    };
    
    
    manipulatorNode.pub.addEventListener('mousedblclick_geom', function(evt) {
      // if the manipulator gets double-clicked on, we create a keyframe in place. 
      variablesNode.setValue(variablesNode.getValue('FABRIC.RT.Xfo', xfoIndex), xfoIndex, xfoTrackFilter);
    });
    
    ///////////////////////////////////////////////////////////
    manipulatorNode.getTargetNode = function() {
      return rigNode;
    };
    manipulatorNode.getParentXfo = function() {
      return transformNode.getParentXfo();
    }
    manipulatorNode.getTargetXfo = function() {
      return manipulatorNode.pub.getVariableXfo();
    }
    manipulatorNode.setTargetXfo = function(xfo) {
      manipulatorNode.pub.setVariableXfo(xfo);
    }
    manipulatorNode.setTargetOri = function(ori) {
      var xfo = this.getTargetXfo();
      xfo.ori = ori;
      this.setTargetXfo(xfo);
    }
    manipulatorNode.getManipulationSpaceXfo = function() {
      return transformNode.pub.getGlobalXfo();
    }
    manipulatorNode.pub.getTargetName = function() {
      return options.targetName;
    }
    
    // Thia function is used to find the closest local axis to
    // the given vec. The local axis can then be used in manipulation.
    // Manipulators can then determine the best local axis based on the
    // camera facing direction. 
    manipulatorNode.findClosestLocalAxis = function(vec) {
      var manipulationSpaceOri = this.getManipulationSpaceXfo().ori,
        localX = manipulationSpaceOri.rotateVector(new FABRIC.RT.Vec3(1, 0, 0)),
        localY = manipulationSpaceOri.rotateVector(new FABRIC.RT.Vec3(0, 1, 0)),
        localZ = manipulationSpaceOri.rotateVector(new FABRIC.RT.Vec3(0, 0, 1)),
        deltaX = vec.angleTo(localX),
        deltaY = vec.angleTo(localY),
        deltaZ = vec.angleTo(localZ);

      if (deltaX > Math.HALF_PI) {
        deltaX = Math.PI - deltaX;
        localX = localX.negate();
      }
      if (deltaY > Math.HALF_PI) {
        deltaY = Math.PI - deltaY;
        localY = localY.negate();
      }
      if (deltaZ > Math.HALF_PI) {
        deltaZ = Math.PI - deltaZ;
        localZ = localZ.negate();
      }
      if (deltaX < deltaY && deltaX < deltaZ) {
        return localX;
      }
      else if (deltaY < deltaX && deltaY < deltaZ) {
        return localY;
      }
      return localZ;
    }

    if(options.rigNode){
       manipulatorNode.pub.setRigNode(options.rigNode);
    }
    return manipulatorNode;
  }});


});
