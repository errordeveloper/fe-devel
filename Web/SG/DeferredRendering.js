/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/Rendering",
               "RT/OGLRenderTarget"], function() {


var capitalizeFirstLetter = function(str) {
  return str[0].toUpperCase() + str.substr(1);
};

FABRIC.SceneGraph.registerNodeType('DeferredPrePassMaterial', {
  briefDesc: 'The ShadowMapMaterial allows to draw a shadow.',
  detailedDesc: 'The ShadowMapMaterial allows to draw a shadow. This is used when working with shadow maps.',
  parentNodeDesc: 'Material',
  factoryFn: function(options, scene) {
    options.parentEventHandler = false;
    var materialNode = scene.constructNode('Material', options);
    
    materialNode.addReferenceInterface('renderer', 'BaseDeferredRenderer',
      function(nodePrivate){
        var shader = scene.getPrivateInterface(materialNode.getShaderNode());
        nodePrivate.getDeferredPrePassEventHandler().appendChildEventHandler(shader.getRedrawEventHandler());
      });
    if(options.rendererNode){
      materialNode.pub.setRendererNode(options.rendererNode);
    }
    return materialNode;
  }});

FABRIC.SceneGraph.registerNodeType('DeferredPostPassMaterial', {
  briefDesc: 'The ShadowMapMaterial allows to draw a shadow.',
  detailedDesc: 'The ShadowMapMaterial allows to draw a shadow. This is used when working with shadow maps.',
  parentNodeDesc: 'Material',
  factoryFn: function(options, scene) {
    options.parentEventHandler = false;
    var materialNode = scene.constructNode('Material', options);
    
    materialNode.addReferenceInterface('renderer', 'BaseDeferredRenderer',
      function(nodePrivate){
        var shader = scene.getPrivateInterface(materialNode.getShaderNode());
        nodePrivate.getDeferredPostPassEventHandler().appendChildEventHandler(shader.getRedrawEventHandler());
        
        var oglRenderTargetTextureNames = nodePrivate.getOglRenderTargetTextureNames();
        var renderTargetTextures = nodePrivate.getRenderTargetTextures();
        for(i = 0; i < renderTargetTextures.length; ++i) {
          var setTextureFuncName = 'set' + capitalizeFirstLetter(oglRenderTargetTextureNames[i]) + 'TextureNode';
          if(materialNode.pub[setTextureFuncName] !== undefined) {
            materialNode.pub[setTextureFuncName]( renderTargetTextures[i].pub );
          }
        }
  
        var materialRedrawHandler = materialNode.getRedrawEventHandler();
  
        if(options.shadeFullScreen !== undefined && options.shadeFullScreen) {
          materialRedrawHandler.postDescendBindings.insert(
            scene.constructOperator({
                operatorName: 'drawShaderQuad',
                srcCode:'use OGLTexture2D, OGLShaderProgram;\n' +
                        'operator drawShaderQuad(io OGLShaderProgram program){ \n' +
                        '  drawScreenQuad(program.programId, Vec2(-1.0,1.0), Vec2(1.0,-1.0), false);}',
                entryPoint: 'drawShaderQuad',
                parameterLayout: [
                  'shader.shaderProgram'
                ]
          }),0);
        }
      });
    if(options.rendererNode){
      materialNode.pub.setRendererNode(options.rendererNode);
    }
    return materialNode;
  }});


FABRIC.SceneGraph.registerNodeType('BaseDeferredRenderer', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: '',
  optionsDesc: {
    colorBuffers: "array of render targets specifying 'nbChannels' (1 to 4) and 'name' for each target, and optionally the internal OpenGL format (by default: 16F per channel). Note that for a target name N, it is expected that the deferred shader will have a " + 
                  "texture parameter name NTexture. Ex: [{nbChannels: 4, name: 'diffuse'}, {nbChannels: 3, name: 'normal', internalFormat: FABRIC.SceneGraph.OpenGLConstants.GL_RGB16F}], matching params diffuseTexture and normalTexture in deferred shader material." +
                  "Note that your prePass shaders should then write to targets of the corresponding indices: glFragData[0]=diffuseColor; glFragData[1]=normal;",
    addDepth: "if depth should be attached",
    showDebug: "Display view of the render targets",
    lightNode: "to be used by the Phong shading pass",
    forwardRenderPass: "enables to mix with forward rendering"
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      showDebug: false,
      addDepth: true,
      forwardRenderPass: false,
      colorBuffers: []
      });

    if(options.colorBuffers.length == 0)
      throw ('Error: no render target provided');

    var deferredRenderNode = scene.constructNode('SceneGraphNode');
    var redrawEventHandler = deferredRenderNode.constructEventHandlerNode('DeferredDraw');
    redrawEventHandler.setScopeName('deferredDraw');

    scene.getSceneRedrawOpaqueObjectsEventHandler().appendChildEventHandler(redrawEventHandler);

    var renderTargetRedrawEventHandler = deferredRenderNode.constructEventHandlerNode('DeferredPrePass');
    redrawEventHandler.appendChildEventHandler(renderTargetRedrawEventHandler);

    var channelToInternalFormat = [0, FABRIC.SceneGraph.OpenGLConstants.GL_R16F, FABRIC.SceneGraph.OpenGLConstants.GL_RG16F, FABRIC.SceneGraph.OpenGLConstants.GL_RGB16F, FABRIC.SceneGraph.OpenGLConstants.GL_RGBA16F];
    var channelToFormat = [0, FABRIC.SceneGraph.OpenGLConstants.GL_RED, FABRIC.SceneGraph.OpenGLConstants.GL_RG, FABRIC.SceneGraph.OpenGLConstants.GL_RGB, FABRIC.SceneGraph.OpenGLConstants.GL_RGBA];

    var oglRenderTargetTextureDescs = [];
    var oglRenderTargetTextureNames = [];

    var i;
    for( i = 0; i < options.colorBuffers.length; ++i ) {
      var nbChannels = options.colorBuffers[i].nbChannels;
      if(nbChannels === undefined || nbChannels < 1 || nbChannels > 4)
        throw ('Error: unsupported number of channels for render target ' + i);
      if(options.colorBuffers[i].name === undefined)
        throw ('Error: name not specified for render target ' + i);

      var internalFormat = options.colorBuffers[i].internalFormat;
      if(internalFormat === undefined)
        internalFormat = channelToInternalFormat[nbChannels];

      oglRenderTargetTextureDescs.push(
          new FABRIC.RT.OGLRenderTargetTextureDesc (
              2, // COLOR_BUFFER
              new FABRIC.RT.OGLTexture2D (
                internalFormat,
                channelToFormat[nbChannels],
                FABRIC.SceneGraph.OpenGLConstants.GL_FLOAT)
            ));
      oglRenderTargetTextureNames.push(options.colorBuffers[i].name);
    }

    var forwardRenderBufferIndex = options.colorBuffers.length;
    if(options.forwardRenderPass) {
      oglRenderTargetTextureDescs.push(
          new FABRIC.RT.OGLRenderTargetTextureDesc (
              2, // COLOR_BUFFER
              new FABRIC.RT.OGLTexture2D (
                FABRIC.SceneGraph.OpenGLConstants.GL_RGBA8,
                channelToFormat[4],
                FABRIC.SceneGraph.OpenGLConstants.GL_FLOAT)
            ));
      oglRenderTargetTextureNames.push('forwardRender');
    }

    if(options.addDepth) {
      oglRenderTargetTextureDescs.push(
          new FABRIC.RT.OGLRenderTargetTextureDesc (
              1, // DEPTH_BUFFER
              new FABRIC.RT.OGLTexture2D (
                FABRIC.SceneGraph.OpenGLConstants.GL_DEPTH_COMPONENT,
                FABRIC.SceneGraph.OpenGLConstants.GL_DEPTH_COMPONENT,
                FABRIC.SceneGraph.OpenGLConstants.GL_FLOAT)
            ));
      oglRenderTargetTextureNames.push('depth');
    }

    var nbRenderTargets = oglRenderTargetTextureNames.length;
    var renderTarget = FABRIC.RT.oglRenderTarget(0,0,
      oglRenderTargetTextureDescs,
      {
        clearColor: options.clearColor
      }
    );
    redrawEventHandler.addMember('renderTarget', 'OGLRenderTarget', renderTarget);

    ///////////////////////////////
    //Pre pass

    if(!options.forwardRenderPass) {
      renderTargetRedrawEventHandler.preDescendBindings.append(
        scene.constructOperator({
            operatorName: 'bindScreenRenderTarget',
            srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
            entryPoint: 'bindScreenRenderTarget',
            parameterLayout: [
              'window.width',
              'window.height',
              'deferredDraw.renderTarget'
            ]
          }));
    } else {
      renderTargetRedrawEventHandler.preDescendBindings.append(
        scene.constructOperator({
            preProcessorDefinitions: {
              FORWARD_RENDER_BUFFER_INDEX: forwardRenderBufferIndex
            },
            operatorName: 'bindPrePassRenderTargets',
            srcCode:  'use OGLRenderTarget;\n' +
                      'operator bindPrePassRenderTargets( io Integer width, io Integer height, io OGLRenderTarget renderTarget ) {\n' +
                      '  renderTarget.prebind(width, height);\n' +
                      '  renderTarget.bindFbo(true);\n' +
                      '  Integer drawBufferIDs[];\n' +
                      '  for(Size i=0; i < FORWARD_RENDER_BUFFER_INDEX; i++){\n' +
                      '      drawBufferIDs.push(Integer(GL_COLOR_ATTACHMENT0 + drawBufferIDs.size()));\n' +
                      '  }\n' +
                      '  renderTarget.bindColorBuffers(drawBufferIDs);\n' +
                      '}\n',
            entryPoint: 'bindPrePassRenderTargets',
            parameterLayout: [
              'window.width',
              'window.height',
              'deferredDraw.renderTarget'
            ]
          }));
    }

    renderTargetRedrawEventHandler.postDescendBindings.append(
      scene.constructOperator({
          operatorName: 'unbindRenderTarget',
          srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
          entryPoint: 'unbindRenderTarget',
          parameterLayout: [
            'deferredDraw.renderTarget'
          ]
        }));

    redrawEventHandler.addMember('debugDrawToggle', 'Boolean', options.showDebug);
    redrawEventHandler.addMember('debugShaderProgID', 'Integer', 0);
    deferredRenderNode.addMemberInterface(redrawEventHandler, 'debugDrawToggle', true);

    redrawEventHandler.postDescendBindings.append(
      scene.constructOperator({
          operatorName: 'debugDrawRenderTargets',
          srcCode:'use OGLTexture2D;\n' +
                  'operator debugDrawRenderTargets(io OGLRenderTarget renderTarget, io Integer progId, io Boolean draw){ \n' +
                  'if(!draw)return;\n' +
                  'Size i, j, n = renderTarget.textures.size, curr = 0;\n' +
                  'for(i = 0; i < 3; ++i){\n' +
                  '  for(j = 0; j < 3; ++j){\n' +
                  '    while(true) {\n' +
                  '      if(curr == n)\n' +
                  '        return;\n' +
                  '      if(renderTarget.textures[curr].type == 2)\n' +
                  '        break;\n' +
                  '      curr++;\n' +
                  '    }\n' +
                  '    Scalar x = -1.0 + Scalar(i)/1.5;\n' +
                  '    Scalar y = 1.0 - Scalar(j)/1.5;\n' +
                  '    renderTarget.textures[curr++].texture.bind(0);\n' +
                  '    drawTexture(0, progId, Vec2(x,y), Vec2(x+0.66666,y-0.66666), false);\n' +
                  '    if(curr == n)\n' +
                  '      return;}}}\n',
          entryPoint: 'debugDrawRenderTargets',
          parameterLayout: [
            'self.renderTarget',
            'self.debugShaderProgID',
            'self.debugDrawToggle'
          ]
    }));
    
    ///////////////////////////////
    //Post (shading) pass
    var shaderPassRedrawEventHandler = deferredRenderNode.constructEventHandlerNode('DeferredPostPass');
    redrawEventHandler.appendChildEventHandler(shaderPassRedrawEventHandler);

    shaderPassRedrawEventHandler.preDescendBindings.append(
      scene.constructOperator({
          operatorName: 'preDeferredRenderShading',
          srcCode: 'use FabricOGL; operator preDeferredRenderShading(){\n'+
                      'glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);\n' +
                      'glDepthMask(GL_FALSE);\n' +
                      'glEnable(GL_BLEND);\n' +
                      'glBlendFunc(GL_SRC_ALPHA, GL_ONE);\n' +
                      '}',
          entryPoint: 'preDeferredRenderShading',
          parameterLayout: []
        }));

    shaderPassRedrawEventHandler.postDescendBindings.append(
      scene.constructOperator({
          operatorName: 'postDeferredRenderShading',
          srcCode: 'use FabricOGL; operator postDeferredRenderShading(){\n'+
                      'glPopAttrib();}',
          entryPoint: 'postDeferredRenderShading',
          parameterLayout: []
        }));

    //Build event handlers for loading each render target
    var renderTargetTextures = [];
    for(i = 0; i < nbRenderTargets; ++i) {
      var imageNode = scene.constructNode('RenderTargetBufferTexture', {
        name: oglRenderTargetTextureNames[i],
        bufferIndex: i
      });
      renderTargetTextures.push(imageNode);
    }

    deferredRenderNode.getOglRenderTargetTextureNames = function(){ return oglRenderTargetTextureNames; }
    deferredRenderNode.getRenderTargetTextures = function(){ return renderTargetTextures; }
    ///////////////////////////////
    //Forward render mix pass

    if(options.forwardRenderPass) {
      var forwardRenderMixRedrawEventHandler = deferredRenderNode.constructEventHandlerNode('ForwardRenderMixPass');
      redrawEventHandler.appendChildEventHandler(forwardRenderMixRedrawEventHandler);
      forwardRenderMixRedrawEventHandler.addMember('shaderProgID', 'Integer', 0);

      forwardRenderMixRedrawEventHandler.preDescendBindings.append(
        scene.constructOperator({
            preProcessorDefinitions: {
              FORWARD_RENDER_BUFFER_INDEX: forwardRenderBufferIndex
            },
            operatorName: 'bindForwardRenderMixRenderTarget',
            srcCode:  'use OGLRenderTarget;\n' +
                      'operator bindForwardRenderMixRenderTarget( io OGLRenderTarget renderTarget ) {\n' +
                      '  renderTarget.bindFbo(false);\n' +
                      '  Integer drawBufferIDs[];\n' +
                      '  drawBufferIDs.push(Integer(GL_COLOR_ATTACHMENT0 + FORWARD_RENDER_BUFFER_INDEX));\n' +
                      '  glClearColor(0.0, 0.0, 0.0, 0.0);\n' +
                      '  renderTarget.bindColorBuffers(drawBufferIDs);\n' +
                      '}\n',
            entryPoint: 'bindForwardRenderMixRenderTarget',
            parameterLayout: [
              'deferredDraw.renderTarget'
            ]
          }));

      forwardRenderMixRedrawEventHandler.postDescendBindings.append(
        scene.constructOperator({
            preProcessorDefinitions: {
              FORWARD_RENDER_BUFFER_INDEX: forwardRenderBufferIndex
            },
            operatorName: 'unbindAndDrawForwardRenderTarget',
            srcCode:  'use OGLRenderTarget; use OGLTexture2D;\n' +
                      'operator unbindAndDrawForwardRenderTarget( io OGLRenderTarget renderTarget, io Integer shaderProgramID ) {\n' +
                      '  renderTarget.unbind();\n' +
                      '  glPushAttrib(GL_ENABLE_BIT);\n' +
                      '  glEnable(GL_BLEND);\n' +
                      '  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);\n' +
                      '  renderTarget.textures[FORWARD_RENDER_BUFFER_INDEX].texture.bind(0);\n' +
                      '  drawTexture(0, shaderProgramID, Vec2(-1.0,1.0), Vec2(1.0,-1.0), false);\n' +
                      '  glPopAttrib();}',
            entryPoint: 'unbindAndDrawForwardRenderTarget',
            parameterLayout: [
              'deferredDraw.renderTarget',
              'self.shaderProgID'
              ]
      }));

      deferredRenderNode.pub.addForwardRenderMaterial = function(materialName, options) {
        options.parentEventHandler = forwardRenderMixRedrawEventHandler;
        return scene.pub.constructNode(materialName, options);
      };
    }

    return deferredRenderNode;
}});


FABRIC.SceneGraph.registerNodeType('PhongDeferredRenderer', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: 'BaseDeferredRenderer',
  optionsDesc: {
    addPhongShadingLayer: "Adds a global Phong shading by default"
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      addPhongShadingLayer: true
      });

    var definedDeferredPhongMaterials = false;
    if( definedDeferredPhongMaterials == false ) {
      FABRIC.SceneGraph.defineEffectFromFile('DeferredPostPhongMaterial', 'FABRIC_ROOT/SG/Shaders/DeferredPostPhongShader.xml');
      definedDeferredPhongMaterials = true;
    }

    options.colorBuffers = [{name: 'positions', nbChannels: 3}, {name: 'normals', nbChannels: 3}, {name: 'diffuseAndSpecularFactor', nbChannels: 4, internalFormat: FABRIC.SceneGraph.OpenGLConstants.GL_RGBA8}];
    options.addDepth = true;

    var deferredRenderNode = scene.constructNode('BaseDeferredRenderer', options);

    // Note: the renderer owns the post render shader.
    // Persistence will not save the post material,
    // unless someone explicitly requested it from the
    // renderer, and then added it to the persistence manager.
    scene.constructNode('DeferredPostPhongMaterial', {
      shadeFullScreen: true,
      rendererNode: deferredRenderNode.pub,
      lightNode: options.lightNode
    });

    return deferredRenderNode;
}});


});

