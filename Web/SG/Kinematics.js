/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph", "RT/Xfo"], function() {

FABRIC.SceneGraph.registerNodeType('Transform', {
  briefDesc: 'The Transform node implements a global or hierarchical transform.',
  detailedDesc: 'The Transform node uses a redraw eventhandler to draw an OpenGL projection. The node can be used in '+
                'a hierarchical mode, in which is projects its own projection into another Transform node\'s projection. '+
                'This allows to compute hierarchies in 3D.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    hierarchical: 'Flag determining wether this Transform node should calculate a globalXfo based on a parents globalXfo',
    parentTransformNode: 'If the TransformNode is hierarchical, this option can optionally provide a parent transform node to immediately assign',
    globalXfo: 'Optional: Specify the intial globalXfo for this transform.',
    localXfo: 'Optional: Specify the intial localXfo for this transform.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        hierarchical: false,
        parentTransformNode: undefined,
        globalXfo: undefined,
        localXfo: undefined
      });

    var transformNode = scene.constructNode('SceneGraphNode', options),
      dgnode = transformNode.constructDGNode('DGNode'),
      textureNode = undefined,
      parentTransformNode,
      children = [];

    dgnode.addMember('globalXfo', 'Xfo', options.globalXfo);
    transformNode.addMemberInterface(dgnode, 'globalXfo');

    if (options.hierarchical) {
      dgnode.addMember('localXfo', 'Xfo', options.localXfo);
      dgnode.bindings.append(scene.constructOperator( {
          operatorName: 'calcGlobalXfo',
          srcFile: 'FABRIC_ROOT/SG/KL/calcGlobalXfo.kl',
          parameterLayout: [
            'self.localXfo',
            'parent.globalXfo',
            'self.globalXfo'
          ],
          entryPoint: 'calcGlobalXfo'
        }));

      transformNode.addMemberInterface(dgnode, 'localXfo', true);

      // use a custom getter
      transformNode.pub.setGlobalXfo = function(val) {
        if (parentTransformNode) {
          var parentXfo = parentTransformNode.pub.getGlobalXfo();
          val = val.multiply(parentXfo.inverse());
          dgnode.setData('localXfo', val);
        }
        else {
          dgnode.setData('globalXfo', val);
        }
      };

      transformNode.addReferenceInterface('Parent', 'Transform',
        function(nodePrivate, reciprocate){
          parentTransformNode = nodePrivate;
          dgnode.setDependency(parentTransformNode.getDGNode(), 'parent');
          if (reciprocate !== false && nodePrivate.pub.addChild) {
            nodePrivate.pub.addChild(this, false);
          }
      });
      transformNode.pub.addChild = function(node, reciprocate) {
        children.push(node);
        if (reciprocate !== false) {
          node.setParentNode(this, false);
        }
      };
      transformNode.pub.getChildren = function() {
        return children;
      };

      if (options.parentTransformNode) {
        transformNode.pub.setParentNode(options.parentTransformNode);
      }
    }else {
      transformNode.pub.setGlobalXfo = function(val) {
        if(val.constructor.name == "Array") {
          dgnode.resize(val.length);
          dgnode.setBulkData({ globalXfo: val});
        }
        else {
          dgnode.setData('globalXfo', 0, val);
        }
      };
    }
    
    //////////////////////////////////////////
    // Persistence
    var parentAddDependencies = transformNode.addDependencies;
    transformNode.addDependencies = function(sceneSerializer) {
      parentAddDependencies(sceneSerializer);
      if(parentTransformNode){
        sceneSerializer.addNode(parentTransformNode.pub);
      }
    };
    
    var parentWriteData = transformNode.writeData;
    var parentReadData = transformNode.readData;
    transformNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      constructionOptions.hierarchical = options.hierarchical;
      if(parentTransformNode){
        nodeData.parentTransformNode = parentTransformNode.pub.getName();
        nodeData.localXfo = transformNode.pub.getLocalXfo();
      }else{
        nodeData.globalXfo = transformNode.pub.getGlobalXfo();
      }
      
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
    };
    transformNode.readData = function(sceneDeserializer, nodeData) {
      if(nodeData.parentTransformNode){
        var parentTransformNode = sceneDeserializer.getNode(nodeData.parentTransformNode);
        if(parentTransformNode){
          transformNode.pub.setParentNode(parentTransformNode);
        }
        if(nodeData.localXfo){
          transformNode.pub.setLocalXfo(nodeData.localXfo);
        }
      }
      else if(nodeData.globalXfo){
        transformNode.pub.setGlobalXfo(nodeData.globalXfo);
      }
      
      parentReadData(sceneDeserializer, nodeData);
    };

    return transformNode;
  }});

FABRIC.SceneGraph.registerNodeType('TransformTexture', {
  briefDesc: 'The TransformTexture node is an Image node which can be used for storing matrices into a texture buffer.',
  detailedDesc: 'The TransformTexture node is an Image node which can be used for storing matrices into a texture buffer. This is used for efficient instance rendering.',
  parentNodeDesc: 'Image',
  optionsDesc: {
    transformNode: 'A sliced transform node storing all of the transform to store.',
    dynamic: 'If set to true, the texture will be reloaded every frame.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      transformNode: undefined,
      dynamic: true
    });
    
    var textureNode = scene.constructNode('Image', options);
    
    var dgnode = textureNode.constructDGNode('DGNode');
    // create the operator to convert the matrices into a texture
    dgnode.addMember('textureMatrix', 'Mat44');
    
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'matchCount',
      srcCode: 'operator matchCount(in Container parentContainer, io Container selfContainer) { selfContainer.resize( parentContainer.size() ); }',
      entryPoint: 'matchCount',
      parameterLayout: [
        'transforms',
        'self'
      ],
      async: false
    }));
    dgnode.bindings.append(scene.constructOperator( {
      operatorName: 'calcGlobalTransposedMatrix',
      srcFile: 'FABRIC_ROOT/SG/KL/loadTransformTexture.kl',
      preProcessorDefinitions: {
        TRANSFORM_TEXTURE_HEIGHT_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('transformTextureHeight')
      },
      parameterLayout: [
        'transforms.globalXfo<>',
        'self.textureMatrix',
        'self.index',
      ],
      entryPoint: 'calcGlobalTransposedMatrix'
    }));
        
    var redrawEventHandler = textureNode.constructEventHandlerNode('Redraw');
    textureNode.getRedrawEventHandler = function() { return redrawEventHandler; }
    
    var tex = FABRIC.RT.oglMatrixBuffer2D();
    tex.forceRefresh = options.dynamic;
      
    redrawEventHandler.addMember('oglTexture2D', 'OGLTexture2D', tex);
    redrawEventHandler.addMember('matricesTempBuffer', 'Mat44[]');
    redrawEventHandler.addMember('textureHeight', 'Size');

    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
      operatorName: 'prepareTextureMatrix',
      srcFile: 'FABRIC_ROOT/SG/KL/loadTransformTexture.kl',
      preProcessorDefinitions: {
        TRANSFORM_TEXTURE_HEIGHT_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('transformTextureHeight')
      },
      entryPoint: 'prepareTextureMatrix',
      parameterLayout: [
        'textureMatrix.textureMatrix<>',
        'self.matricesTempBuffer',
        'self.textureHeight',
        'self.oglTexture2D',
        'shader.shaderProgram',
        'textureStub.textureUnit'
      ]
    }));
    
    redrawEventHandler.setScope('textureMatrix', dgnode);
    
    
    textureNode.pub.setTransformNode = function(node) {
      if (!node.isTypeOf('Transform')) {
        throw ('Incorrect type assignment. Must assign a Transform');
      }
      var transformNode = scene.getPrivateInterface(node);
      dgnode.setDependency(transformNode.getDGNode(), 'transforms');
    };
    
    if(options.transformNode) {
      textureNode.pub.setTransformNode(options.transformNode);
    }

    return textureNode;
  }});

FABRIC.SceneGraph.registerNodeType('AimTransform', {
  briefDesc: 'The AimTransform node implements a global lookat transform.',
  detailedDesc: 'The AimTransform uses the Transform node to implement a lookat transform, using a '+
                'root position as well as a target position. This node is used for the TargetCamera node.',
  parentNodeDesc: 'Transform',
  optionsDesc: {
    target: 'The target position for the initial direction of the Transform'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        target: new FABRIC.RT.Vec3(0, 0, 0)
      });
    
    if(options.position && options.target){
      options.globalXfo = new FABRIC.RT.Xfo({ tr: options.position });
      options.globalXfo.ori.setFromDirectionAndUpvector(
        options.position.subtract(options.target),
        new FABRIC.RT.Vec3(0,1,0));
    }

    var aimTransformNode = scene.constructNode('Transform', options);
    var dgnode = aimTransformNode.getDGNode();
    dgnode.addMember('target', 'Vec3', options.target);

    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'aimTransform',
      srcFile: 'FABRIC_ROOT/SG/KL/aimTransform.kl',
      entryPoint: 'aimTransform',
      parameterLayout: [
        'self.globalXfo',
        'self.target'
      ]
    }));

    aimTransformNode.addMemberInterface(dgnode, 'target', true);
    return aimTransformNode;
  }});

});

