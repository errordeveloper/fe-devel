/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "RT/OGLTexture2D",
               "RT/OGLTexture3D",
               "RT/OGLTextureCube"
               ],
              function() {

FABRIC.SceneGraph.registerNodeType('Image', {
  briefDesc: 'The Texture node is an Image node which can be used for texturing of a 3D object.',
  detailedDesc: 'The Texture node is an Image node which can be used for texturing of a 3D object.',
  parentNodeDesc: 'SceneGraphNode',
  factoryFn: function(options, scene) {
    var imageNode = scene.constructNode('SceneGraphNode', options);
    return imageNode;
  }});


FABRIC.SceneGraph.registerNodeType('RenderTargetBufferTexture', {
  briefDesc: 'The RenderTargetBufferTexture is used to bind render target buffers to be used as textures.',
  detailedDesc: 'The RenderTargetBufferTexture is used to bind render target buffers to be used as textures.',
  parentNodeDesc: 'Image',
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      bufferIndex: 0
    });
    var textureNode = scene.constructNode('Image', options);
    var redrawEventHandler = textureNode.constructEventHandlerNode('Redraw');
    
    redrawEventHandler.addMember('bufferIndex', 'Integer', options.bufferIndex)
    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
      operatorName: 'bindRenderTargetBufferAsTexture',
      srcCode: 'use OGLRenderTarget; operator bindTexture( io OGLRenderTarget renderTarget, io Integer bufferIndex, io Integer textureUnit ) {\n' +
                   ' renderTarget.textures[bufferIndex].texture.bind(textureUnit);\n\
                }',
      entryPoint: 'bindTexture',
      parameterLayout: [
        'deferredDraw.renderTarget',
        'self.bufferIndex',
        'textureStub.textureUnit'
      ]
    }));
    return textureNode;
  }});


FABRIC.SceneGraph.registerNodeType('Image2D', {
  briefDesc: 'The Image node holds image data, and optionally creates an URL image loader and an OpenGL texture.',
  detailedDesc: 'The Image node holds generic image data (members: pixels, width, height). ' +
                'If \'options.createResourceLoadNode\', an URL-based image loader will be incorporated, and currently supports ' +
                '.png, .tga, .exr and .hdr image formats. If \'options.createLoadTextureEventHandler\', an OpenGL texture ' +
                'will be created from the image.',
  parentNodeDesc: 'Image',
  optionsDesc: {
    format: 'Pixel format. Currently supported: RGB, RGBA, Color and Scalar.',
    createDgNodes: 'If this is set to true the Image node will contain a dgnode to store the pixel data.',
    createResourceLoadNode: 'Set to true this flag will enable the Image node to load a texture off a resource load node.',
    createLoadTextureEventHandler: 'If the image uses a ResouceLoadNode and this flag is set, it will create an EventHandler for the Image being loaded.',
    width: 'The width of the empty Image',
    height: 'The height of the empty Image',
    color: 'The standard color for the empty Image',
    url: 'The URL to load the Image from',
    forceRefresh: 'If this is set, the Image will always be re-loaded onto the GPU. This is useful for animated Images.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      format: 'RGBA',
      createDgNodes: false,
      createResourceLoadNode: true,
      createLoadTextureEventHandler: true,
      initImage: true,
      width: 128,
      height: 128,
      color: FABRIC.RT.rgba(0.0,0.0,0.0,0.0),
      url: undefined,
      forceRefresh: false,
      glRepeat: true
    });
    
    var resourceLoadNode, resourceloaddgnode, oglTexture;
    
    // check pixel format and setup the oglTexture
    if(options.format == 'RGBA') {
      oglTexture = FABRIC.RT.oglTexture2D();
    } else if(options.format == 'RGB') {
      oglTexture = FABRIC.RT.oglTexture2D();
      oglTexture.glInternalFormat = FABRIC.SceneGraph.OpenGLConstants.GL_RGB8;
      oglTexture.glFormat = FABRIC.SceneGraph.OpenGLConstants.GL_RGB;
    } else if(options.format == 'Color') {
      oglTexture = FABRIC.RT.oglTexture2D_Color();
    } else if(options.format == 'Scalar') {
      oglTexture = FABRIC.RT.oglDepthBuffer2D();
    } else
      throw('Pixel format "'+options.format+'" is not support by Image2D!');
    if(!options.glRepeat) {
      oglTexture.wrapS = FABRIC.SceneGraph.OpenGLConstants.GL_CLAMP;
      oglTexture.wrapT = FABRIC.SceneGraph.OpenGLConstants.GL_CLAMP;
    }
    oglTexture.forceRefresh = options.forceRefresh;
    
    // create the base node
    var imageNode = scene.constructNode('Image', options);
    if(options.createDgNodes){
      var uniformsdgnode = imageNode.constructDGNode('UniformsDGNode');
      var pixelsdgnode = imageNode.constructDGNode('PixelsDGNode');
      uniformsdgnode.addMember('width', 'Size', options.createResourceLoadNode ? undefined : options.width);
      uniformsdgnode.addMember('height', 'Size', options.createResourceLoadNode ? undefined : options.height);
      uniformsdgnode.addMember('pixels', options.format+'[]');
      pixelsdgnode.addMember('pixels', options.format);
      pixelsdgnode.setDependency(uniformsdgnode,'uniforms');

      imageNode.addMemberInterface(uniformsdgnode, 'width');
      imageNode.addMemberInterface(uniformsdgnode, 'height');
    }

    if (options.createResourceLoadNode) {
      resourceLoadNode = scene.constructNode('ResourceLoad', options);
      resourceloaddgnode = resourceLoadNode.getDGLoadNode();
      if(options.createDgNodes && options.url){
        imageNode.getPixelsDGNode().setDependency(resourceloaddgnode, 'resource');
        imageNode.getPixelsDGNode().bindings.append(scene.constructOperator({
          operatorName: 'loadSlicedImage'+options.format,
          parameterLayout: [
            'self',
            'resource.resource',
            'uniforms.width',
            'uniforms.height',
            'self.pixels<>'
          ],
          preProcessorDefinitions: { PIXELFORMAT: options.format },
          entryPoint: 'loadSlicedImage'+options.format,
          srcFile: 'FABRIC_ROOT/SG/KL/loadTexture.kl'
        }));
      };

      imageNode.pub.getResourceLoadNode = function() {
        return resourceLoadNode.pub;
      };

      imageNode.pub.isImageLoaded = function() {
        return resourceLoadNode ? resourceLoadNode.pub.isLoaded() : false;
      };
    } else {
      if(options.createDgNodes && options.initImage && options.width && options.height){
        imageNode.getUniformsDGNode().addMember('color', options.format, options.color);
        imageNode.getUniformsDGNode().addMember('initiated', 'Boolean', false);
        imageNode.getPixelsDGNode().bindings.append(scene.constructOperator({
          operatorName: 'initImageFrom'+options.format,
          parameterLayout: [
            'self',
            'uniforms.width',
            'uniforms.height',
            'uniforms.color',
            'uniforms.initiated',
            'self.pixels<>'
          ],
          preProcessorDefinitions: { PIXELFORMAT: options.format },
          entryPoint: 'initImageFrom'+options.format,
          srcFile: 'FABRIC_ROOT/SG/KL/loadTexture.kl'
        }));

        imageNode.pub.setColor = function(color) {
          imageNode.getUniformsDGNode().setData('initiated', 0, false);
          if( color.getType() === 'FABRIC.RT.Color' ) {
            var byteColor = new FABRIC.RT.RGBA();
            byteColor.setFromScalarColor(color);
            color = byteColor;
          }
          imageNode.getUniformsDGNode().setData('color', 0, color );
        }
      };
    }

    if (options.createLoadTextureEventHandler) {
      // Construct the handler for loading the image into texture memory.
      var redrawEventHandler = imageNode.constructEventHandlerNode('Redraw');
      redrawEventHandler.addMember('oglTexture2D', 'OGLTexture2D', oglTexture);
      if(options.createDgNodes){
        redrawEventHandler.setScope('uniforms', imageNode.getUniformsDGNode());
        redrawEventHandler.setScope('image', imageNode.getPixelsDGNode());
        redrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'bindTexture'+options.format,
          srcFile: 'FABRIC_ROOT/SG/KL/loadTexture.kl',
          preProcessorDefinitions: { PIXELFORMAT: options.format },
          entryPoint: 'bindTexture'+options.format,
          parameterLayout: [
            'uniforms.width',
            'uniforms.height',
            'image.pixels<>',
            'self.oglTexture2D',
            'textureStub.textureUnit'
          ]
        }));
      }
      else if(options.createResourceLoadNode){
        redrawEventHandler.setScope('resource', resourceloaddgnode);
        redrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'loadAndBindTexture'+options.format,
          srcFile: 'FABRIC_ROOT/SG/KL/loadTexture.kl',
          preProcessorDefinitions: { PIXELFORMAT: options.format },
          entryPoint: 'loadAndBindTexture'+options.format,
          parameterLayout: [
            'resource.resource',
            'self.oglTexture2D',
            'textureStub.textureUnit'
          ]
        }));
      }
    }
    

    imageNode.pub.getUrl = function() {
      return resourceLoadNode ? resourceLoadNode.pub.getUrl() : '';
    };

    imageNode.pub.setUrl = function(url) {
      if(resourceLoadNode) {
        resourceLoadNode.pub.setUrl(url);
      }
    };
    //////////////////////////////////////////
    // Persistence
    var parentWriteData = imageNode.writeData;
    var parentReadData = imageNode.readData;
    imageNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      for (var i in options) {
        constructionOptions[i] = options[i];
      }
      constructionOptions.url = imageNode.pub.getUrl();
    };
    imageNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
    };

    return imageNode;
  }});

FABRIC.SceneGraph.registerNodeType('Image3D', {
  briefDesc: 'The Image node holds 3D image data, and optionally creates an URL image loader and an OpenGL texture.',
  detailedDesc: 'The Image node holds generic image data (members: pixels, width, height, depth), which might be color or grayscale. ' +
                'If \'options.createResourceLoadNode\', an URL-based image loader will be incorporated, and currently supports ' +
                '(todo) image formats. If \'options.createLoadTextureEventHandler\', an OpenGL 3D texture ' +
                'will be created from the image.',
  parentNodeDesc: 'Image',
  optionsDesc: {
    format: 'Pixel format. Currently supported: RGBA, UShort, Byte, Color, Scalar.',
    createDgNodes: 'If this is set to true the Image node will contain a dgnode to store the pixel data.',
    createResourceLoadNode: 'Set to true this flag will enable the Image node to load a texture off a resource load node.',
    createLoadTextureEventHandler: 'If the image uses a ResouceLoadNode and this flag is set, it will create an EventHandler for the Image being loaded.',
    width: 'The width of the empty Image',
    height: 'The height of the empty Image',
    depth: 'The depth of the empty Image',
    color: 'The standard color for the empty Image',
    url: 'The URL to load the Image from'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      format: 'UShort',
      createDgNodes: true,
      createResourceLoadNode: true,
      createLoadTextureEventHandler: true,
      initImage: true,
      width: 0,
      height: 0,
      depth: 0,
      url: undefined,
      glRepeat: true
    });
    if(options.color === undefined) {
      if(options.format === 'RGBA')
        options.color = new FABRIC.RT.RGBA(0,0,0,0);
      else if(options.format === 'Color')
        options.color = new FABRIC.RT.Color(0,0,0,0);
      else
        options.color = 0;
    }
    var resourceLoadNode, resourceloaddgnode;
    
    var imageNode = scene.constructNode('Image', options);
    if(options.createDgNodes){
      var dgnode = imageNode.constructDGNode('DGNode')
      dgnode.addMember('width', 'Size', options.width);
      dgnode.addMember('height', 'Size', options.height);
      dgnode.addMember('depth', 'Size', options.depth);
      if(options.format === 'UShort')
        dgnode.addMember('pixels', 'Byte[]');
      else
        dgnode.addMember('pixels', options.format + '[]');
  
      imageNode.addMemberInterface(dgnode, 'width');
      imageNode.addMemberInterface(dgnode, 'height');
      imageNode.addMemberInterface(dgnode, 'depth');
    }

    if (options.createResourceLoadNode) {
      if(options.format !== 'UShort')
        throw ('Only UShort 3D textures are currently supported');

      resourceLoadNode = scene.constructNode('ResourceLoad', options);
      resourceloaddgnode = resourceLoadNode.getDGLoadNode();
      if(options.createDgNodes){
        dgnode.addMember('xfoMat', 'Mat44');
        dgnode.setDependency(resourceloaddgnode, 'resource');
        dgnode.bindings.append(scene.constructOperator({
          operatorName: 'load3DImageUShortData',
          parameterLayout: [
            'resource.resource',
            'self.width',
            'self.height',
            'self.depth',
            'self.pixels',
            'self.xfoMat'
          ],
          entryPoint: 'load3DImageUShortData',
          srcFile: 'FABRIC_ROOT/SG/KL/load3DTexture.kl'
        }));
      };

      imageNode.pub.getResourceLoadNode = function() {
        return resourceLoadNode.pub;
      };

      imageNode.pub.isImageLoaded = function() {
        return resourceLoadNode ? resourceLoadNode.pub.isLoaded() : false;
      };
    } else {
      if(options.createDgNodes && options.initImage && options.width && options.height && options.depth){
        if(options.format === 'UShort')
          dgnode.addMember('color', 'Byte', options.color);
        else
          dgnode.addMember('color', options.format, options.color);
        dgnode.addMember('initiated', 'Boolean', false);
        dgnode.bindings.append(scene.constructOperator({
          operatorName: 'initImageFrom' + options.format,
          parameterLayout: [
            'self.width',
            'self.height',
            'self.depth',
            'self.color',
            'self.pixels',
            'self.initiated'
          ],
          entryPoint: 'initImageFrom' + options.format,
          srcFile: 'FABRIC_ROOT/SG/KL/load3DTexture.kl'
        }));
      };
    }

    var redrawEventHandler;
    if (options.createLoadTextureEventHandler) {
      // Construct the handler for loading the image into texture memory.
      redrawEventHandler = imageNode.constructEventHandlerNode('Redraw');
      var oglTexture;

      if(options.format === 'Byte')
         oglTexture = FABRIC.RT.oglTexture3D_Byte();
      else if(options.format === 'Color')
         oglTexture = FABRIC.RT.oglTexture3D_Color();
      else if(options.format === 'Scalar')
         oglTexture = FABRIC.RT.oglTexture3D_Scalar();
      else if(options.format === 'UShort')
         oglTexture = FABRIC.RT.oglTexture3D_UShort();
      else
         oglTexture = FABRIC.RT.oglTexture3D_RGBA();

      if(!options.glRepeat) {
        oglTexture.wrapS = FABRIC.SceneGraph.OpenGLConstants.GL_CLAMP;
        oglTexture.wrapT = FABRIC.SceneGraph.OpenGLConstants.GL_CLAMP;
        oglTexture.wrapR = FABRIC.SceneGraph.OpenGLConstants.GL_CLAMP;
      }
      redrawEventHandler.addMember('oglTexture3D', 'OGLTexture3D', oglTexture);
      redrawEventHandler.addMember('forceSingleRefresh', 'Boolean', false);

      if(options.createDgNodes){

        if(resourceLoadNode) {
          redrawEventHandler.addMember('currUrl', 'String');
          redrawEventHandler.setScope('resource', resourceloaddgnode);
          redrawEventHandler.preDescendBindings.append(scene.constructOperator({
            operatorName: 'detectResourceChange',
            srcCode: 'operator detectResourceChange(io FabricResource resource, io String prevUrl, io Boolean refresh){\n' + 
                          '  if(prevUrl != resource.url && (resource.data.size() != 0 || Boolean(resource.dataExternalLocation))){' +
                          '    prevUrl = resource.url;\n' +
                          '    refresh = true;} }',
            entryPoint: 'detectResourceChange',
            parameterLayout: [
              'resource.resource',
              'self.currUrl',
              'self.forceSingleRefresh'
            ]
          }));
        }

        redrawEventHandler.setScope('image', dgnode);
        redrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'bind' + options.format + 'Texture3D',
          srcFile: 'FABRIC_ROOT/SG/KL/load3DTexture.kl',
          entryPoint: 'bind' + options.format + 'Texture3D',
          parameterLayout: [
            'image.width',
            'image.height',
            'image.depth',
            'image.pixels',
            'self.oglTexture3D',
            'textureStub.textureUnit',
            'self.forceSingleRefresh'
          ]
        }));
      }
      else if(options.createResourceLoadNode){
        redrawEventHandler.setScope('resource', resourceloaddgnode);
        redrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'loadAndBindUShortTexture',
          srcFile: 'FABRIC_ROOT/SG/KL/load3DTexture.kl',
          entryPoint: 'loadAndBindUShortTexture',
          parameterLayout: [
            'resource.resource',
            'self.oglTexture3D',
            'textureStub.textureUnit'
          ]
        }));
      }
    }

    imageNode.pub.getUrl = function() {
      return resourceLoadNode ? resourceLoadNode.pub.getUrl() : '';
    };

    imageNode.pub.setUrl = function(url) {
      if(resourceLoadNode) {
        resourceLoadNode.pub.setUrl(url);
      }
    };
    
    var parentWriteData = imageNode.writeData;
    var parentReadData = imageNode.readData;
    imageNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      constructionOptions.createDgNodes = options.createDgNodes;
      constructionOptions.createResourceLoadNode = options.createResourceLoadNode;
      constructionOptions.createLoadTextureEventHandler = options.createLoadTextureEventHandler;
      if(resourceLoadNode){
        nodeData.url = resourceLoadNode.pub.getUrl();
      }
    };
    imageNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      if(nodeData.url){
        imageNode.pub.setUrl(nodeData.url);
      }
    };

    return imageNode;
  }});

FABRIC.SceneGraph.registerNodeType('CubeMap', {
  briefDesc: 'The CubeMap node contains 6 Image nodes which can be used to texture with cubic mapping. ',
  detailedDesc: 'The CubeMap node contains 6 Image nodes which can be used to texture with cubic mapping. ' +
                'This means it requires six images to be aligned in a cubic fashion.',
  parentNodeDesc: 'Image',
  optionsDesc: {
    urls: 'An array of six URLs to the six images to load.',
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      urls: []
    });
    
    var cubMapNode = scene.constructNode('Image', options);
    var redrawEventHandler = cubMapNode.constructEventHandlerNode('Redraw');
    redrawEventHandler.addMember('cubeMap', 'OGLTextureCube', FABRIC.RT.cubeMapLDR());
    
    var createTextureLoaderNode = function(url, faceId){
      var resourceLoadNode = scene.constructNode('ResourceLoad', { url: url });
      var resourceloaddgnode = resourceLoadNode.getDGLoadNode();
      redrawEventHandler.setScope('resource'+faceId, resourceloaddgnode);
    }
    createTextureLoaderNode(options.urls[0], 0);
    createTextureLoaderNode(options.urls[1], 1);
    createTextureLoaderNode(options.urls[2], 2);
    createTextureLoaderNode(options.urls[3], 3);
    createTextureLoaderNode(options.urls[4], 4);
    createTextureLoaderNode(options.urls[5], 5);

    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
        operatorName: 'bindCubeMap',
        srcFile: 'FABRIC_ROOT/SG/KL/loadCubeMap.kl',
        entryPoint: 'bindCubeMap',
        parameterLayout: [
          'resource0.resource',
          'resource1.resource',
          'resource2.resource',
          'resource3.resource',
          'resource4.resource',
          'resource5.resource',
          'self.cubeMap',
          'textureStub.textureUnit'
        ]
      }));
    return cubMapNode;
  }});

FABRIC.SceneGraph.registerNodeType('Video', {
  briefDesc: 'The Video node is an Image node that changes over time.',
  detailedDesc: 'The Video node uses the ffmpeg extension to load animated video footage into an Image node. ' +
                'For that the image content is repeatedly copied onto the GPU.',
  parentNodeDesc: 'Image2D',
  optionsDesc: {
    localFileName: 'The Video node currently only supports a local filename, not an URL.',
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        url: '',
        nbCachedFrames: 0,
        animationControllerNode: undefined
      });

    // ensure to use the right settings for video
    options.createResourceLoadNode = true;
    options.createLoadTextureEventHandler = false;
    options.createDgNodes = false;
    options.initImage = false;
    options.format = 'RGB';

    // check if we have an url
    if (!options.url) {
        throw ('You need to specify a valid url for a video node!');
    }

    var videoNode = scene.constructNode('Image2D', options);
    // add all members
    var dgnode = scene.getPrivateInterface(videoNode.pub.getResourceLoadNode()).getDGLoadNode();
    dgnode.addMember('handle', 'VideoHandle');
    dgnode.addMember('pixels', 'RGB[]');

    videoNode.addMemberInterface(dgnode, 'handle');
  
    // decode the resource
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'videoLoadResource',
      srcFile: 'FABRIC_ROOT/SG/KL/loadVideo.kl',
      entryPoint: 'videoLoadResource',
      parameterLayout: [
        'self.resource',
        'self.handle'
      ]
    }));

    var animationController = undefined;
    var timeBinding;
    videoNode.addReferenceInterface('AnimationController', 'AnimationController',
      function(nodePrivate){
        animationController = nodePrivate;
        var animationControllerDGNode = animationController.getDGNode();
        dgnode.setDependency(animationControllerDGNode, 'controller');
        
      });
    
    // use a dict for storing the already parsed frames
    dgnode.addMember('pixelCache','RGB[][]');
    dgnode.addMember('pixelCacheIndex','Integer[]');
    dgnode.addMember('pixelCacheLimit','Integer',options.nbCachedFrames);
    
    // Here the timerange of the controller is set using the video time range.
    // Note: this is not strictly legal in our dependency graph model
    // as we should not be modifying our dependencies data.
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'videoSetTimeRange',
      srcFile: 'FABRIC_ROOT/SG/KL/loadVideo.kl',
      entryPoint: 'videoSetTimeRange',
      parameterLayout: [
        'self.handle',
        'controller.timeRange',
      ]
    }));
    
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'videoSeekTimeCached',
      srcFile: 'FABRIC_ROOT/SG/KL/loadVideo.kl',
      entryPoint: 'videoSeekTime',
      parameterLayout: [
        'self.handle',
        'controller.localTime',
        'self.pixels',
        'self.pixelCache',
        'self.pixelCacheIndex',
        'self.pixelCacheLimit'
      ]
    }));

    var redrawEventHandler = videoNode.constructEventHandlerNode('Redraw');
    redrawEventHandler.setScope('video', dgnode);
    redrawEventHandler.addMember('bufferID', 'Integer', 0);
    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
        operatorName: 'videoLoadToGPU',
        srcFile: 'FABRIC_ROOT/SG/KL/loadVideo.kl',
        entryPoint: 'videoLoadToGPU',
        parameterLayout: [
          'video.handle',
          'video.pixels',
          'self.bufferID',
          'textureStub.textureUnit'
        ]
    }));
    
    // extend public interface
    if(options.animationControllerNode){
      videoNode.pub.setAnimationControllerNode(options.animationControllerNode);
    }
    else{
      videoNode.pub.setAnimationControllerNode(scene.constructNode('AnimationController').pub);
    }

    return videoNode;
  }});

FABRIC.SceneGraph.registerNodeType('PointSpriteTexture', {
  briefDesc: 'The PointSpriteTexture is an Image node that can be used to texture a point using a sprite.',
  detailedDesc: 'The PointSpriteTexture is an Image node that can be used to texture a point using a sprite. ' +
                'You can specify the resolution of the sprite.',
  parentNodeDesc: 'Image',
  optionsDesc: {
    spriteResolution: 'The resolution of the sprite texture.',
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        spriteResolution: 32
      });
    
    var pointSpriteTextureNode = scene.constructNode('Image', options);
    
    var dgnode = pointSpriteTextureNode.constructDGNode('DGNode');
    dgnode.addMember('resolution', 'Integer', options.spriteResolution);

    var redrawEventHandler = pointSpriteTextureNode.constructEventHandlerNode('Redraw');
    redrawEventHandler.setScope('image', dgnode);
    redrawEventHandler.addMember('bufferID', 'Integer', 0);
    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
            operatorName: 'createSpriteTexture',
            srcFile: 'FABRIC_ROOT/SG/KL/loadTexture.kl',
            entryPoint: 'createSpriteTexture',
            preProcessorDefinitions: { PIXELFORMAT: 'RGBA' },
            parameterLayout: [
              'image.resolution',
              'self.bufferID',
              'textureStub.textureUnit'
            ]
          }));

    return pointSpriteTextureNode;
  }});


FABRIC.SceneGraph.registerNodeType('ScreenGrab', {
  briefDesc: 'The ScreenGrab node stores the viewport to an Image node.',
  detailedDesc: 'The ScreenGrab node stores the viewport to an Image node by encoding PNG using a ResourceLoadNode. ',
  parentNodeDesc: 'SceneGraphNode',
  factoryFn: function(options, scene) {
    //TO refine: right now this node is grabbing a screenshot constantly!
    //We should include a way to 'mute' and 'unmute' it. The problem
    //is how to know when it is filled with content; an event should be sent. Can
    //we do this without modifying the core?
    var screenGrabNode = scene.constructNode('SceneGraphNode', options);
    var screenGrabEvent = screenGrabNode.constructEventNode('ScreenGrabEvent');
    var screenGrabEventHandler = screenGrabNode.constructEventHandlerNode('ScreenGrab');
    
    screenGrabEvent.appendEventHandler(screenGrabEventHandler);
    screenGrabEventHandler.addMember('width', 'Size');
    screenGrabEventHandler.addMember('height', 'Size');
    screenGrabEventHandler.addMember('pixels', 'RGBA[]');
    screenGrabEventHandler.addMember('resource', 'FabricResource');
    
    var viewportNode;
    screenGrabNode.addReferenceInterface('Viewport', 'Viewport',
      function(nodePrivate){
        viewportNode = nodePrivate;
        screenGrabEventHandler.setScope('window', viewportNode.getFabricWindowObject().windowNode);
    });
    
    screenGrabEventHandler.postDescendBindings.append(scene.constructOperator({
      operatorName: 'grabViewport',
      srcFile: 'FABRIC_ROOT/SG/KL/grabViewport.kl',
      entryPoint: 'grabViewport',
      parameterLayout: [
        'self.width',
        'self.height',
        'self.pixels',
        'window.fboId'
      ],
      async: false
    }));

    screenGrabEventHandler.postDescendBindings.append(scene.constructOperator({
      operatorName: 'encodeImage',
      srcFile: 'FABRIC_ROOT/SG/KL/encodeImage.kl',
      entryPoint: 'encodeImageLDR',
      parameterLayout: [
        'self.width',
        'self.height',
        'self.pixels',
        'self.resource'
      ],
      async: false
    }));

    screenGrabNode.pub.saveAs = function() {
      screenGrabEvent.fire();
      try {
        var userFileHandle = FABRIC.IO.queryUserFileHandle(FABRIC.IO.forSave, 'Save Screen Grab Image As...', 'png', 'fabricScreenGrab');
        screenGrabEventHandler.putResourceToFile(userFileHandle,'resource');
      }
      catch (e) { }
    };

    var writeFileOperatorAttached = false;
    screenGrabNode.pub.writeImageFile = function(path) {
      if(!writeFileOperatorAttached){
        screenGrabEventHandler.addMember('path', 'String', path);
        screenGrabEventHandler.postDescendBindings.append(scene.constructOperator({
          operatorName: 'writeFile',
          srcFile: 'FABRIC_ROOT/SG/KL/writeFile.kl',
          entryPoint: 'writeFile',
          parameterLayout: [
            'self.resource',
            'self.path'
          ],
          async: false
        }));
        writeFileOperatorAttached = true;
      }else{
        screenGrabEventHandler.setData('path', 0, path);
      }
      screenGrabEvent.fire();
    };
    if(options.viewport){
      screenGrabNode.pub.setViewportNode(options.viewport);
    }
    
    return screenGrabNode;
  }
});

});

