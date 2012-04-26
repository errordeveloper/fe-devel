/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph"], function() {

// Note: the ids defined in this table are used in the KL/GLSL code.
var Lights = {
  types: {
    'PointLight': 0,
    'DirectionalLight': 1,
    'SpotLight': 2
  }
};

FABRIC.SceneGraph.registerNodeType('Light', {
  briefDesc: 'The Light node is an abstract base node used by the Light types in the SceneGraph.',
  detailedDesc: 'The Light node is an abstract base node used by the Light types in the SceneGraph.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    color: 'The color of the light when rendered in OpenGL. Note: Not all shaders use the light color',
    transformNode: 'The Transform node used to control the position of the light in the scene',
    globalXfo: 'If a transfrom node is not specified, then a default one is constructed, and this is the initial value of the globalXfo on that Transform node.'
  },
  factoryFn: function (options, scene) {
    scene.assignDefaults(options, {
      color: FABRIC.RT.rgb(1.0, 1.0, 1.0),
      transformNode: 'Transform',
      globalXfo: new FABRIC.RT.Xfo()
    });

    if (options.lightType == undefined) {
      throw ': Lights must define a type';
    }

    var lightNode = scene.constructNode('SceneGraphNode', options),
      dgnode = lightNode.constructDGNode('DGNode'),
      transformNode,
      redrawEventHandler;
    
    dgnode.addMember('type', 'Integer', options.lightType);
    dgnode.addMember('lightMat44', 'Mat44');
    dgnode.addMember('cameraMat44', 'Mat44');
    dgnode.addMember('color', 'Color', options.color);

    lightNode.getRedrawEventHandler = function() {
      // This call will replace the 'getRedrawEventHandler' with an accessor.
      redrawEventHandler = lightNode.constructEventHandlerNode('Redraw');
      redrawEventHandler.setScope('light', dgnode);

      var Materials = FABRIC.require("SG/Materials");
      redrawEventHandler.preDescendBindings.append(scene.constructOperator({
        operatorName: 'loadLight',
        srcFile: 'FABRIC_ROOT/SG/KL/loadLights.kl',
        preProcessorDefinitions: {
          LIGHTTYPE_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightType'),
          LIGHTCOLOR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightColor'),
          LIGHTPOSITION_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightPosition'),
          LIGHTDIR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightDir'),
          LIGHTCOSCUTOFF_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightCosCutoff'),
          LIGHTVIEWMATRIX_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightShadowMapMatrix')
        },
        entryPoint: 'loadLight',
        parameterLayout: [
        'shader.shaderProgram',
        'light.type',
        'light.color',
        'camera.cameraMat44',
        'light.lightMat44'
      ]
      }));
      

      if (options.castShadows) {
        
        redrawEventHandler.addMember('shadowMap', 'Size', 0);

        redrawEventHandler.preDescendBindings.append(
          scene.constructOperator({
              operatorName: 'loadLightMatrixUniform',
              srcFile: 'FABRIC_ROOT/SG/KL/loadLights.kl',
              preProcessorDefinitions: {
                LIGHTTYPE_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightType'),
                LIGHTCOLOR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightColor'),
                LIGHTPOSITION_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightPosition'),
                LIGHTDIR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightDir'),
                LIGHTCOSCUTOFF_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightCosCutoff'),
                LIGHTVIEWMATRIX_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightShadowMapMatrix')
              },
              entryPoint: 'loadLightMatrixUniform',
              parameterLayout: [
                'shader.shaderProgram',
                'light.shadowMat44',
                'camera.cameraMat44'
              ]
            }));
        
        redrawEventHandler.preDescendBindings.append(
          scene.constructOperator({
              operatorName: 'bindShadowMapBufferOp',
              srcFile: 'FABRIC_ROOT/SG/KL/shadowMaps.kl',
              entryPoint: 'bindShadowMapBuffer',
              parameterLayout: [
                'self.shadowMap',
                'light.depthRenderTarget'
              ]
            }));
      }
      return redrawEventHandler;
    };
    
    if (options.castShadows) {
      // Construct the event handler that renders the depth buffer. 
      var shadowRenderEventHandler = lightNode.constructEventHandlerNode('RenderDepthBuffer');

      // Projection Values
      dgnode.addMember('nearDistance', 'Scalar', options.nearDistance);
      dgnode.addMember('farDistance', 'Scalar', options.farDistance);
      dgnode.addMember('projectionMat44', 'Mat44');
      dgnode.addMember('shadowMat44', 'Mat44');
      dgnode.addMember('depthRenderTarget', 'OGLRenderTarget', FABRIC.RT.oglDepthRenderTarget(options.resolution));
      
      shadowRenderEventHandler.setScope('light', dgnode);
      // override the camera scope with the lights.
      // This causes all rendering to occur from the lights pov.
      shadowRenderEventHandler.setScope('camera', dgnode); 
     
      shadowRenderEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'bindDepthRenderTarget',
          srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
          entryPoint: 'bindRenderTarget',
          parameterLayout: [
            'light.depthRenderTarget'
          ]
        }));
      shadowRenderEventHandler.postDescendBindings.append(scene.constructOperator({
          operatorName: 'unbindDepthRenderTarget',
          srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
          entryPoint: 'unbindRenderTarget',
          parameterLayout: [
            'light.depthRenderTarget'
          ]
        }));
       
      if(options.displayShadowDebug === true){
        // Display the shadow color map on screen.
        var shadowDebugRenderEventHandler = lightNode.constructEventHandlerNode('renderDebugQuad');
        shadowDebugRenderEventHandler.addMember('program', 'Integer');
        scene.getScenePostRedrawEventHandler().appendChildEventHandler(shadowDebugRenderEventHandler);
        shadowDebugRenderEventHandler.setScope('light', dgnode);
        shadowDebugRenderEventHandler.preDescendBindings.append(
          scene.constructOperator({
              operatorName:"debugShadowMapBuffer",
              srcFile:"FABRIC_ROOT/SG/KL/shadowMaps.kl",
              entryPoint:"debugShadowMapBuffer",
              parameterLayout:[
                'light.depthRenderTarget',
                'self.program'
              ]
            }));
      }

      scene.registerShadowCastingLightSourceHandler(shadowRenderEventHandler);
    }
    
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'loadLightXfo',
      srcCode: 'use Xfo; use Mat44; operator loadLightXfo(io Xfo xfo, io Mat44 lightMat44, io Mat44 cameraMat44){ lightMat44 = xfo.toMat44(); cameraMat44 = lightMat44.inverse(); }',
      entryPoint: 'loadLightXfo',
      parameterLayout: [
        'transform.globalXfo',
        'self.lightMat44',
        'self.cameraMat44',
      ]
    }));
    
    // public interface
    // TODO: try to have base class with input transform (would share with cameraNode)
    lightNode.addReferenceInterface('Transform', 'Transform',
      function(nodePrivate){
        transformNode = nodePrivate;
        dgnode.setDependency(transformNode.getDGNode(), 'transform');
      });

    lightNode.addMemberInterface(dgnode, 'color', true);
    lightNode.addMemberInterface(dgnode, 'cameraMat44');

    if (typeof options.transformNode == 'string') {
      lightNode.pub.setTransformNode(scene.constructNode(options.transformNode, {
          hierarchical: false,
          globalXfo:options.globalXfo
        }).pub);
    } else {
      lightNode.pub.setTransformNode(options.transformNode);
    }

    return lightNode;
  }});


FABRIC.SceneGraph.registerNodeType('PointLight', {
  briefDesc: 'The PointLight node represents a point light in the SceneGraph.',
  detailedDesc: 'The PointLight node represents a point light in the SceneGraph. A Point light casts light in all directions from a point in space.',
  parentNodeDesc: 'Light',
  optionsDesc: {
    position: 'The initial positions of the Light if a Transform node is not specified',
    display: 'Toggle whether this light should display an icon of itself in the screen.',
    displaySize: 'The size of the displayed icon'
  },
  factoryFn: function (options, scene) {

    scene.assignDefaults(options, {
      position: new FABRIC.RT.Vec3(420.0, 1000.0, 600.0),
      display: false,
      displaySize: 5
    });

    options.lightType = Lights.types.PointLight;
    options.globalXfo = new FABRIC.RT.Xfo({ tr: options.position });
    var pointLightNode = scene.constructNode('Light', options);
    var dgnode = pointLightNode.getDGNode();
    
    pointLightNode.pub.getPosition = function() {
      return pointLightNode.pub.getTransformNode().getGlobalXfo().tr;
    }
    pointLightNode.pub.setPosition = function(v) {
      pointLightNode.pub.getTransformNode().setGlobalXfo(new FABRIC.RT.Xfo({ tr: v }));
    }

    if (options.display === true) {
      //Ideally we should merge all these line segments together... or have a meta-geometry generator in kl (able to create circles, lines, etc)?
      var materialNode = scene.pub.constructNode('FlatMaterial', {
          color: FABRIC.RT.color(0.2, 0.5, 0.8, 1.0)
      });
      var circleNode = scene.pub.constructNode('Circle', {
          radius: options.displaySize/2
      });
      scene.pub.constructNode('Instance', {
        transformNode: pointLightNode.pub.getTransformNode(),
        geometryNode: circleNode,
        materialNode: materialNode
      });

      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: pointLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({ ori: new FABRIC.RT.Quat().setFromAxisAndAngle(FABRIC.RT.Vec3.xAxis, Math.HALF_PI) })
        }),
        geometryNode: circleNode,
        materialNode: materialNode
      });

      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: pointLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({ ori: new FABRIC.RT.Quat().setFromAxisAndAngle(FABRIC.RT.Vec3.zAxis, Math.HALF_PI) })
        }),
        geometryNode: circleNode,
        materialNode: materialNode
      });
    }

    return pointLightNode;
  }});

FABRIC.SceneGraph.registerNodeType('DirectionalLight', {
  briefDesc: 'The DirectionalLight node represents a directional light in the SceneGraph.',
  detailedDesc: 'The DirectionalLight node represents a directional light in the SceneGraph.' +
                'A Directional Light casts light in only one direction and the position is not used in lighting calculations.',
  parentNodeDesc: 'Light',
  optionsDesc: {
    position: 'The initial position of the Light if a Transform node is not specified',
    direction: 'The initial direction of the Light if a Transform node is not specified',
    display: 'Toggle whether this light should display an icon of itself in the screen.',
    displaySize: 'The size of the displayed icon'
  },
  factoryFn: function (options, scene) {

    if (!options.transformNode) {
      scene.assignDefaults(options, {
        direction: new FABRIC.RT.Vec3(0.0, 0.737, 0.737),
        position: new FABRIC.RT.Vec3(100.0, 100.0, 100.0),
        displaySize: 5,
        size: 40.0
      });
    }

    options.lightType = Lights.types.DirectionalLight;
    options.globalXfo = new FABRIC.RT.Xfo();
    if (options.position) {
      //The position is not used for directionalLight's lighting, but might be used for displaying the light
      options.globalXfo.tr = new FABRIC.RT.Vec3(options.position);
    }
    if (options.direction) {
      options.globalXfo.ori = new FABRIC.RT.Quat().setFrom2Vectors(
          new FABRIC.RT.Vec3(0.0, 0.0, -1.0),
          new FABRIC.RT.Vec3(options.direction).unit(),
          true
        );
    }
    var directionalLightNode = scene.constructNode('Light', options);
    var dgnode = directionalLightNode.getDGNode();

    var parentGetRedrawEventHandler = directionalLightNode.getRedrawEventHandler;
    directionalLightNode.getRedrawEventHandler = function() {
      var redrawEventHandler = parentGetRedrawEventHandler();

      redrawEventHandler.preDescendBindings.append(scene.constructOperator({
        operatorName: 'loadDirectionalLight',
        srcFile: 'FABRIC_ROOT/SG/KL/loadLights.kl',
        preProcessorDefinitions: {
          LIGHTTYPE_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightType'),
          LIGHTCOLOR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightColor'),
          LIGHTPOSITION_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightPosition'),
          LIGHTDIR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightDir'),
          LIGHTCOSCUTOFF_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightCosCutoff'),
          LIGHTVIEWMATRIX_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightShadowMapMatrix')
        },
        entryPoint: 'loadDirectionalLight',
        parameterLayout: [
          'shader.shaderProgram',
          'camera.cameraMat44',
          'light.lightMat44'
        ]
      }));
      return redrawEventHandler;
    };


    if (options.castShadows) {
      dgnode.addMember('size', 'Scalar', options.size);
      dgnode.bindings.append(scene.constructOperator({
          operatorName: 'calcDirectionalLightProjectionMatrices',
          srcFile: 'FABRIC_ROOT/SG/KL/shadowMaps.kl',
          entryPoint: 'calcDirectionalLightProjectionMatrices',
          parameterLayout: [
            'self.nearDistance',
            'self.farDistance',
            'self.size',
            'self.cameraMat44',
            'self.projectionMat44',
            'self.shadowMat44'
          ]
        }));
    }
    
    if (options.display === true) {
      //Ideally we should merge all these line segments together... or have a meta-geometry generator in kl (able to create circles, lines, etc)?
      var materialNode = scene.pub.constructNode('FlatMaterial', {
          color: FABRIC.RT.color(0.2, 0.5, 0.8, 1.0)
      });
      var circleNode = scene.pub.constructNode('Circle', {
          radius: options.size * 0.5
      });
      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: directionalLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({ ori: new FABRIC.RT.Quat().setFromAxisAndAngle(FABRIC.RT.Vec3.xAxis, Math.HALF_PI) })
        }),
        geometryNode: circleNode,
        materialNode: materialNode
      });

      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: directionalLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({ ori: new FABRIC.RT.Quat().setFromAxisAndAngle(FABRIC.RT.Vec3.xAxis, Math.HALF_PI), tr: new FABRIC.RT.Vec3(0, 0, -options.size) })
        }),
        geometryNode: circleNode,
        materialNode: materialNode
      });
    }

    return directionalLightNode;
  }});


FABRIC.SceneGraph.registerNodeType('SpotLight', {
  briefDesc: 'The SpotLight node represents a spot light in the SceneGraph.',
  detailedDesc: 'The SpotLight node represents a spot light in the SceneGraph. ' +
                'A Spot Light casts light in a cone orriginating from a specified position and with an angle specified by the cone angle. ' +
                'The spot light can also be used to cast shadows using shadowmaps which are rendered prior to the scene being rendered',
  parentNodeDesc: 'Light',
  optionsDesc: {
    coneAngle: 'The angle of the cone in radians used in lighting calculations',
    castShadows: 'Toggle determining whether this light will be configured to render shadow maps in the pre render stage.',
    nearDistance: 'The near distance used to define the frustrum when rendering shadow maps',
    farDistance: 'The far distance used to define the frustrum when rendering shadow maps',
    resolution: 'The resolution of the shadow map.',
    displayShadowDebug: 'If set to true, renders a rectangle in the top left of the screen displaying the shadow map',
    position: 'The initial position of the Light if a Transform node is not specified',
    target: 'The initial target of the Light if a Transform node is not specified',
    display: 'Toggle whether this light should display an icon of itself in the screen.',
    displaySize: 'The size of the displayed icon'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        coneAngle: Math.degToRad(60),
        castShadows: true,
        nearDistance: 1,
        farDistance: 1000,
        resolution: 1024,
        displayShadowDebug: false,
        display: false,
        displaySize: 50
      });

    if (!options.transformNode) {
      scene.assignDefaults(options, {
        position: new FABRIC.RT.Vec3(100.0, 100.0, 100.0),
        target: new FABRIC.RT.Vec3(0.0, 0.0, 0.0)
      });
      options.globalXfo = new FABRIC.RT.Xfo({
        tr: new FABRIC.RT.Vec3(options.position),
        ori: new FABRIC.RT.Quat().setFrom2Vectors(
          new FABRIC.RT.Vec3(0.0, 0.0, -1.0),
          new FABRIC.RT.Vec3(options.target).subtract(options.position).unit(),
          true
        )
      });
    }

    options.lightType = Lights.types.SpotLight;
    var spotLightNode = scene.constructNode('Light', options);
    var dgnode = spotLightNode.getDGNode();
    
    dgnode.addMember('coneAngle', 'Scalar', options.coneAngle);
    spotLightNode.addMemberInterface(dgnode, 'coneAngle', true);

    var parentGetRedrawEventHandler = spotLightNode.getRedrawEventHandler;
    spotLightNode.getRedrawEventHandler = function() {
      var redrawEventHandler = parentGetRedrawEventHandler();

      redrawEventHandler.preDescendBindings.append(
        scene.constructOperator({
          operatorName: 'loadSpotLight',
          srcFile: 'FABRIC_ROOT/SG/KL/loadLights.kl',
          preProcessorDefinitions: {
            LIGHTTYPE_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightType'),
            LIGHTCOLOR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightColor'),
            LIGHTPOSITION_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightPosition'),
            LIGHTDIR_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightDir'),
            LIGHTCOSCUTOFF_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightCosCutoff'),
            LIGHTVIEWMATRIX_ATTRIBUTE_ID: FABRIC.SceneGraph.getShaderParamID('lightShadowMapMatrix')
          },
          entryPoint: 'loadSpotLight',
          parameterLayout: [
            'shader.shaderProgram',
            'light.coneAngle',
            'camera.cameraMat44',
            'light.lightMat44'
          ]
        }));
      return redrawEventHandler;
    };
    
    if (options.castShadows) {
      dgnode.bindings.append(scene.constructOperator({
          operatorName: 'calcSpotLightProjectionMatrices',
          srcFile: 'FABRIC_ROOT/SG/KL/shadowMaps.kl',
          entryPoint: 'calcSpotLightProjectionMatrices',
          parameterLayout: [
            'self.nearDistance',
            'self.farDistance',
            'self.coneAngle',
            'self.cameraMat44',
            'self.projectionMat44',
            'self.shadowMat44'
          ]
        }));
    }
    
    spotLightNode.constructDisplay = function() {
      var coneDist = options.displaySize;
      if(options.position && options.target){
        coneDist = options.position.distanceTo(options.target);
      }
      var coneRadius = Math.tan(options.coneAngle * 0.5) * coneDist;
      var lightMaterial = scene.pub.constructNode('FlatMaterial', { color: FABRIC.RT.rgb(1.0, 0.7, 0.4) });
      var crossGeometry = scene.pub.constructNode('Cross', { size: coneDist * 0.05 });

      scene.pub.constructNode('Instance', {
        transformNode: spotLightNode.pub.getTransformNode(),
        geometryNode: crossGeometry,
        materialNode: lightMaterial
      });
      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: spotLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({ tr: new FABRIC.RT.Vec3(0, 0, -coneDist) })
        }),
        geometryNode: crossGeometry,
        materialNode: lightMaterial
      });
      scene.pub.constructNode('Instance', {
        transformNode: scene.pub.constructNode('Transform', {
          hierarchical: true,
          parentTransformNode: spotLightNode.pub.getTransformNode(),
          localXfo: new FABRIC.RT.Xfo({
            ori: new FABRIC.RT.Quat().setFromAxisAndAngle(FABRIC.RT.Vec3.xAxis, Math.HALF_PI),
            tr: new FABRIC.RT.Vec3(0, 0, -coneDist)
          })
        }),
        geometryNode: scene.pub.constructNode('Circle', {
          radius: coneRadius
        }),
        materialNode: lightMaterial
      });
    }
    
    if (options.display === true) {
      spotLightNode.constructDisplay();
    }

    return spotLightNode;
  }});

  return Lights;
});

