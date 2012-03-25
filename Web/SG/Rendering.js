/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Cameras",
               "RT/Vec2",
               "RT/Vec3",
               "RT/Color",
               "RT/Ray",
               "RT/OGLRenderTarget"], function() {


FABRIC.SceneGraph.registerNodeType('Viewport', {
  briefDesc: 'The Viewport node implements the basic OpenGL canvas.',
  detailedDesc: 'Utilizing a redraw eventhandler, the ViewPort node offers a powerful OpenGL canvas, ' +
                'which is connected to an embed element inside the DOM. The most important parameter of '+
                'the ViewPort node\'s options is the windowElement, the ID of the HTML element to append '+
                'the viewport to.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    windowElement: 'The HTML element to attach the viewport to.',
    cameraNode: 'The cameraNode to use for this viewport',
    enableMouseEvents: 'Set to true this ensures to create the mouse down, up and move events.',
    enableRaycasting: 'Set to true this enables raycasting for selection of 3D objects.',
    mouseUpEvents: 'Set to true this enables the mouse up event',
    mouseMoveEvents: 'Set to true this enables the mouse move event',
    backgroundColor: 'The background color of the viewport used for glClearColor',
    postProcessEffect: 'An optional PostProcessEffect node to be used after drawing the viewport, undefined if None.',
    rayIntersectionThreshold: 'The treshold of raycast intersections, typicall below 1.0'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        windowElement: undefined,
        cameraNode: undefined,
        enableMouseEvents: true,
        enableRaycasting: false,
        mouseUpEvents: true,
        mouseMoveEvents: true,
        backgroundColor: FABRIC.RT.rgb(0.5, 0.5, 0.5),
        postProcessEffect: undefined,
        rayIntersectionThreshold: 0.1,
        checkOpenGL2Support: true
      });

    if (!options.windowElement) {
      throw ('Must provide a window to this constructor');
    }

    var cameraNode = undefined, fabricwindow;
    var raycastingEnabled = false;
    var loading = true;
    var windowElement = options.windowElement;
    var viewportNode = scene.constructNode('SceneGraphNode', options),
      dgnode = viewportNode.constructDGNode('DGNode'),
      redrawEventHandler = viewportNode.constructEventHandlerNode('Redraw');
      
    dgnode.addMember('backgroundColor', 'Color', options.backgroundColor);
    
    viewportNode.addMemberInterface(dgnode, 'backgroundColor', true);

    redrawEventHandler.setScope('viewPort', dgnode);

    redrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'viewportBeginRender',
          srcFile: 'FABRIC_ROOT/SG/KL/viewport.kl',
          entryPoint: 'viewportBeginRender',
          parameterLayout: [
            'window.width',
            'window.height',
            'viewPort.backgroundColor'
          ]
        }));

    var fabricwindow = scene.bindViewportToWindow(windowElement, viewportNode);
    var windowRedrawEventHandler = viewportNode.constructEventHandlerNode('WindowRedraw');
    
    fabricwindow.windowNode.addMember('numDrawnVerticies', 'Size');
    fabricwindow.windowNode.addMember('numDrawnTriangles', 'Size');
    fabricwindow.windowNode.addMember('numDrawnGeometries', 'Size');
    fabricwindow.windowNode.addMember('fboId', 'Integer', -1);
    viewportNode.addMemberInterface(fabricwindow.windowNode, 'numDrawnVerticies', false);
    viewportNode.addMemberInterface(fabricwindow.windowNode, 'numDrawnTriangles', false);
    viewportNode.addMemberInterface(fabricwindow.windowNode, 'numDrawnGeometries', false);
            
    windowRedrawEventHandler.setScope('window', fabricwindow.windowNode);
    windowRedrawEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'windowBeginRender',
          srcFile: 'FABRIC_ROOT/SG/KL/window.kl',
          entryPoint: 'windowBeginRender',
          parameterLayout: [
            'window.numDrawnVerticies',
            'window.numDrawnTriangles',
            'window.numDrawnGeometries',
            'window.fboId'
          ]
        }));
    
    var initialLoad = true;
    var visible = false;
    var startLoadMode = function() {
      fabricwindow.hide();
      FABRIC.appendOnResolveAsyncTaskCallback(function(label, countRemaining){
        if(countRemaining===0){

          if(initialLoad) {
            initialLoad = false;
            loading = false;
            fabricwindow.redrawEvent.appendEventHandler(windowRedrawEventHandler);
            if(scene.getScenePreRedrawEventHandler()){
              windowRedrawEventHandler.appendChildEventHandler(scene.getScenePreRedrawEventHandler());
            }
            windowRedrawEventHandler.appendChildEventHandler(redrawEventHandler);
            if(scene.getScenePostRedrawEventHandler()){
              windowRedrawEventHandler.appendChildEventHandler(scene.getScenePostRedrawEventHandler());
            }
            if(raycastingEnabled){
              // the sceneRaycastEventHandler propogates the event throughtout the scene.
              viewPortRaycastEventHandler.appendChildEventHandler(scene.getSceneRaycastEventHandler());
            }
        
            // These functions cannot be called during the initial construction of the
            // graph because they rely on an OpenGL context being set up, and this occurs
            // during the 1st redraw.
            viewportNode.pub.getOpenGLVersion = fabricwindow.getOpenGLVersion;
            viewportNode.pub.getGlewSupported = fabricwindow.getGlewSupported;
            viewportNode.pub.getGlewSupported = fabricwindow.getGlewSupported;
          }

          viewportNode.pub.show();
          return true;
        }
      });
    };
    startLoadMode();
    
    
    var browserZoom = 1.0;
    var retrieveBrowserZoom = function(){
      browserZoom = fabricwindow.windowNode.getData('width') / windowElement.clientWidth;
    }
    viewportNode.pub.show = function(){
      fabricwindow.show();
      visible = true;
      // without the following line the animation review demo loads with a grey screen on OsX.
      // TOOD: figure out why this is required. 
      retrieveBrowserZoom();
      fabricwindow.needsRedraw();
      viewportNode.pub.fireEvent('show');
    };
    viewportNode.pub.hide = function(){
      fabricwindow.hide();
      visible = false;
      viewportNode.pub.fireEvent('hide');
    };
    window.addEventListener('resize', retrieveBrowserZoom, false);
    
    viewportNode.pub.getWidth = function(){  return windowElement.offsetWidth * browserZoom;  };
    viewportNode.pub.getHeight = function(){ return windowElement.offsetHeight * browserZoom; };
    
    var propagationRedrawEventHandler = viewportNode.constructEventHandlerNode('DrawPropagation');
    redrawEventHandler.appendChildEventHandler(propagationRedrawEventHandler);

    // Texture Stub for loading Background textures.
    var backgroundTextureNode, textureStub;
    textureStub = viewportNode.constructEventHandlerNode('BackgroundTextureStub');
    propagationRedrawEventHandler.appendChildEventHandler(textureStub);

    var postProcessEffects = [];

    ///////////////////////////////////////////////////////////////////
    // Raycasting
    var viewPortRaycastEvent, viewPortRaycastEventHandler, viewPortRayCastDgNode;
    var raycastingConstructed = false;

    viewportNode.pub.enableRaycasting = function() {
      if( !raycastingEnabled && scene.getSceneRaycastEventHandler() ) {
        raycastingEnabled = true;
        if( !raycastingConstructed ) {
          raycastingConstructed = true;
          viewPortRayCastDgNode = viewportNode.constructDGNode('RayCastDgNodeDGNode');
          viewPortRayCastDgNode.addMember('x', 'Integer');
          viewPortRayCastDgNode.addMember('y', 'Integer');
          viewPortRayCastDgNode.addMember('ray', 'Ray');
          viewPortRayCastDgNode.addMember('threshold', 'Scalar', options.rayIntersectionThreshold);
          viewPortRayCastDgNode.setDependency(fabricwindow.windowNode, 'window');

          // this operator calculates the rayOri and rayDir from the scopes collected so far.
          // The scopes should be the window, viewport, camera and projection.
          viewPortRayCastDgNode.bindings.append(scene.constructOperator({
            operatorName: 'viewportUpdateRayCast',
            srcFile: 'FABRIC_ROOT/SG/KL/viewport.kl',
            entryPoint: 'viewportUpdateRayCast',
            parameterLayout: [
              'camera.cameraMat44',
              'camera.projectionMat44',
              'window.width',
              'window.height',
              'self.x',
              'self.y',
              'self.ray'
            ]
          }));

          viewPortRaycastEventHandler = viewportNode.constructEventHandlerNode('Raycast');
          viewPortRaycastEventHandler.setScope('raycastData', viewPortRayCastDgNode);
          viewPortRaycastEvent = viewportNode.constructEventNode('RaycastEvent');
          viewPortRaycastEvent.setSelectType('RayIntersection');

          // Raycast events are fired from the viewport. As the event
          // propagates down the tree it collects scopes and fires operators.
          // The operators us the collected scopes to calculate the ray.
          viewPortRaycastEvent.appendEventHandler(viewPortRaycastEventHandler);
          
          // During load we do not connect up the event tree,
          // the registered callback will make the connection
          if( !loading )
            viewPortRaycastEventHandler.appendChildEventHandler(scene.getSceneRaycastEventHandler());
        }
      }
    };

    viewportNode.pub.disableRaycasting = function() {
      if( raycastingEnabled ) {
        raycastingEnabled = false;
      }
    };

    var getElementCoords = function(evt) {
      retrieveBrowserZoom();
      if (evt.offsetX != undefined) {
        // Webkit
        return new FABRIC.RT.Vec2(Math.floor(evt.offsetX*browserZoom), Math.floor(evt.offsetY*browserZoom));
      }
      else if (evt.layerX != undefined) {
        // Firefox
        return new FABRIC.RT.Vec2(Math.floor(evt.layerX*browserZoom), Math.floor(evt.layerY*browserZoom));
      }
      throw("Unsupported Browser");
    }
    
    viewportNode.getElementCoords = function(evt) {
      return getElementCoords(evt);
    };
    
    viewportNode.getWindowElement = function() {
      return windowElement;
    };
    viewportNode.getFabricWindowObject = function() {
      return fabricwindow;
    };
    
    viewportNode.addReferenceInterface('Camera', 'Camera',
      function(nodePrivate){
      // remove the child event handler first
        if(cameraNode != undefined) {
          propagationRedrawEventHandler.removeChildEventHandler(cameraNode.getRedrawEventHandler());
        }
        cameraNode = nodePrivate;
        propagationRedrawEventHandler.appendChildEventHandler(cameraNode.getRedrawEventHandler());
        if (viewPortRayCastDgNode) {
          viewPortRayCastDgNode.setDependency(cameraNode.getDGNode(), 'camera');
        }
      });
    
    
    viewportNode.addReferenceInterface('BackgroundTexture', 'Image2D',
      function(nodePrivate){
        if (textureStub.postDescendBindings.getLength() == 0) {
          textureStub.setScopeName('textureStub');
          textureStub.addMember('textureUnit', 'Integer', 0);
          textureStub.addMember('program', 'Integer', 0);
          textureStub.postDescendBindings.append(
            scene.constructOperator({
                operatorName: 'drawTextureFullScreen',
                srcFile: 'FABRIC_ROOT/SG/KL/drawTexture.kl',
                entryPoint: 'drawTextureFullScreen',
                parameterLayout: [
                  'self.textureUnit',
                  'self.program'
                ]
              }
           ));
        }
        if (backgroundTextureNode) {
          textureStub.removeChildEventHandler(backgroundTextureNode.getRedrawEventHandler());
        }
        backgroundTextureNode = nodePrivate;
        textureStub.appendChildEventHandler(backgroundTextureNode.getRedrawEventHandler());
      });
    viewportNode.addReferenceListInterface('PostProcessEffect', 'PostProcessEffect',
      function(nodePrivate, index){
        var parentEventHandler;
        if (postProcessEffects.length > 0) {
          parentEventHandler = postProcessEffects[postProcessEffects.length - 1].getRedrawEventHandler();
        }
        else {
          parentEventHandler = redrawEventHandler;
        }
        parentEventHandler.removeChildEventHandler(propagationRedrawEventHandler);
        parentEventHandler.appendChildEventHandler(nodePrivate.getRedrawEventHandler());
  
        nodePrivate.getRedrawEventHandler().appendChildEventHandler(propagationRedrawEventHandler);
        postProcessEffects.push(nodePrivate);
      },
      function(nodePrivate, index) {
        var parentEventHandler, childEventHandler;
        postProcessEffects.splice(index, 1);
        if(filterIndex < postProcessEffects.length){
          childEventHandler = postProcessEffects[filterIndex].getRedrawEventHandler();
        }
        else{
          childEventHandler = propagationRedrawEventHandler;
        }
        nodePrivate.getRedrawEventHandler().removeChildEventHandler(childEventHandler);
        
        if (filterIndex > 0) {
          parentEventHandler = postProcessEffects[filterIndex - 1].getRedrawEventHandler();
        }
        else {
          parentEventHandler = redrawEventHandler;
        }
        parentEventHandler.removeChildEventHandler(nodePrivate.getRedrawEventHandler());
        if (filterIndex < postProcessEffects.length) {
          parentEventHandler.appendChildEventHandler(postProcessEffects[filterIndex].getRedrawEventHandler());
        }
        else {
          parentEventHandler.appendChildEventHandler(propagationRedrawEventHandler);
        }
      });
    
    viewportNode.pub.rayCast = function(evt, options) {
      if(!visible){
        return;
      }
      var result = {
        rayData: undefined
      };
      options = scene.assignDefaults(options, {
          returnOnlyClosestNode: true
        });
      if( raycastingEnabled ) {
        var elementCoords = getElementCoords(evt);
        viewPortRayCastDgNode.setData('x', elementCoords.x);
        viewPortRayCastDgNode.setData('y', elementCoords.y);
        var nodes = viewPortRaycastEvent.select();
        result.rayData = viewPortRayCastDgNode.getData('ray');
        if(nodes.length > 0){
          if (options.returnOnlyClosestNode) {
            for (var i = 0; i < nodes.length; i++) {
              if (!result.closestNode || nodes[i].value.distance < result.closestNode.value.distance) {
                result.closestNode = nodes[i];
              }
            }
          }else {
            result.nodes = nodes;
          }
        }
      }
      return result;
    };
    
    viewportNode.pub.calcRayFromMouseEvent = function(evt) {
      var elementCoords = getElementCoords(evt);
      viewPortRayCastDgNode.setData('x', elementCoords.x);
      viewPortRayCastDgNode.setData('y', elementCoords.y);
      viewPortRayCastDgNode.evaluate();
      var ray = viewPortRayCastDgNode.getData('ray');
      return ray;
    };
    
    viewportNode.pub.redraw = function(force) {
      if(!visible){
        return;
      }
      if(!scene.isPlaying() || force){
        // By setting a timout of 0ms, we let the browser provess any pending JavaScript
        setTimeout(function(){
          fabricwindow.needsRedraw();
        }, 0);
      }
    };
    
    viewportNode.pub.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      nodeData.camera = cameraNode.getName();
    };
    
    viewportNode.pub.readData = function(sceneDeserializer, nodeData) {
      if (nodeData.camera) {
        this.setCameraNode(sceneDeserializer.getNode(nodeData.camera));
      }
    };
    
    viewportNode.pub.getFPS = function() {
      // TODO: once we have support for multiple viewports, we should
      // re-write this function.
      return scene.getContext().VP.viewPort.getFPS();
    };
    
    
    var screenGrabber;
    viewportNode.pub.takeScreenShot = function(filename){
      if(!screenGrabber){
        screenGrabber = scene.pub.constructNode('ScreenGrab' );
      }
      screenGrabber.writeImageFile(filename);
    }
    
    if (options.enableRaycasting)
      viewportNode.pub.enableRaycasting();

    if (options.postProcessEffect) {
      viewportNode.pub.addPostProcessEffectShader(options.postProcessEffect);
    }

    if (options.enableMouseEvents) {
      ///////////////////////////////////////////////////////////////////
      // Add Mouse Handling Events
      var mouseOverNode;
      var mouseOverNodeData;
      var propagateEvent = true;
      var bindEventProperties = function(evt) {
        evt.mouseScreenPos  = getElementCoords(evt);
        evt.scene = scene.pub;
        evt.viewportNode = viewportNode.pub;
        if (cameraNode) {
          evt.cameraNode = cameraNode.pub;
        }
        propagateEvent = true;
        var stopPropagation = evt.stopPropagation;
        evt.stopPropagation = function() {
          propagateEvent = false;
          stopPropagation.call(evt);
        }
      }

      var fireEvent = function(name, evt, targetNode) {
        bindEventProperties(evt);
        if (cameraNode) {
          cameraNode.pub.fireEvent(name, evt);
        }
        if (propagateEvent) viewportNode.pub.fireEvent(name, evt);
        if (propagateEvent) scene.pub.fireEvent(name, evt);
      }

      // The mouse has interacted in some way with a geometry.
      // fire an event for objects to handle thier own interaction.
      var fireGeomEvent = function(name, evt, targetNode) {
        bindEventProperties(evt);
        evt.targetNode = targetNode.pub;
        evt.targetNode.fireEvent(name, evt);
        evt.viewportNode = viewportNode;
        if (propagateEvent) cameraNode.pub.fireEvent(name, evt);
        if (propagateEvent) viewportNode.pub.fireEvent(name, evt);
        if (propagateEvent) scene.pub.fireEvent(name, evt);
      }

      var mouseMoveTime;
      var mouseMoveFn = function(evt) {
        mouseMoveTime = (new Date).getTime();
        propagateEvent = true;
        if (cameraNode && viewPortRayCastDgNode && options.mouseMoveEvents) {
          var raycastResult = viewportNode.pub.rayCast(evt);
          if (raycastResult && raycastResult.closestNode) {
            var hitNode = raycastResult.closestNode.node.sceneGraphNode;
            evt.rayData = raycastResult.rayData;
            evt.hitData = raycastResult.closestNode.value;
            if (mouseOverNode == undefined ||
                mouseOverNode.pub.getName() !== hitNode.pub.getName()) {
              if (mouseOverNode) {
                evt.toNode = hitNode;
                evt.hitData = mouseOverNodeData;
                fireGeomEvent('mouseout_geom', evt, mouseOverNode);
              }
              evt.fromElement = evt.relatedTarget = mouseOverNode;
              fireGeomEvent('mouseover_geom', evt, hitNode);
              mouseOverNode = hitNode;
              mouseOverNodeData = evt.hitData;
            }else {
              fireGeomEvent('mousemove_geom', evt, hitNode);
            }
          }
          else {
            if (mouseOverNode) {
              evt.hitData = mouseOverNodeData;
              fireGeomEvent('mouseout_geom', evt, mouseOverNode);
              mouseOverNode = undefined;
            }
          }
        }
        if(propagateEvent){
          fireEvent('mousemove', evt);
        }
      };
      var mouseDownTime;
      var mouseDownFn = function(evt) {
        if(((new Date).getTime() - mouseDownTime) < 200){
          // generate a double-click if the mouse goes down 2x in less than 200ms.
          mouseDblClickFn(evt);
          return;
        }
        propagateEvent = true;
        if (cameraNode && viewPortRayCastDgNode) {
          var raycastResult = viewportNode.pub.rayCast(evt);
          if (raycastResult && raycastResult.closestNode) {
            var hitNode = raycastResult.closestNode.node.sceneGraphNode;
            evt.rayData = raycastResult.rayData;
            evt.hitData = raycastResult.closestNode.value;
            fireGeomEvent('mousedown_geom', evt, hitNode);
          }
        }
        if(propagateEvent){
          fireEvent('mousedown', evt);
        }
        mouseDownTime = (new Date).getTime();
      };

      var mouseUpFn = function(evt) {
        propagateEvent = true;
        if (cameraNode && viewPortRayCastDgNode && options.mouseUpEvents) {
          var raycastResult = viewportNode.pub.rayCast(evt);
          if (raycastResult && raycastResult.closestNode) {
            var hitNode = raycastResult.closestNode.node.sceneGraphNode;
            evt.rayData = raycastResult.rayData;
            evt.hitData = raycastResult;
            fireGeomEvent('mouseup_geom', evt, hitNode);
          }
        }
        if(propagateEvent){
          fireEvent('mouseup', evt);
        }
      };
      
      var mouseDblClickFn = function(evt) {
        propagateEvent = true;
        if (cameraNode && viewPortRayCastDgNode) {
          var raycastResult = viewportNode.pub.rayCast(evt);
          if (raycastResult && raycastResult.closestNode) {
            var hitNode = raycastResult.closestNode.node.sceneGraphNode;
            evt.rayData = raycastResult.rayData;
            evt.hitData = raycastResult.closestNode.value;
            fireGeomEvent('mousedblclick_geom', evt, hitNode);
          }
        }
      };


      // In cases wehre mouse events cost a lot, we can restrict firing to mouse down and moue up.
      windowElement.addEventListener('mousemove', mouseMoveFn, false);
      windowElement.addEventListener('mousedown', mouseDownFn, false);
      windowElement.addEventListener('mouseup', mouseUpFn, false);
      
      scene.pub.addEventListener('beginmanipulation', function(evt){
        // During manipulation we disable raycasting
        viewportNode.pub.disableRaycasting();
      });
      scene.pub.addEventListener('endmanipulation', function(evt){
        viewportNode.pub.enableRaycasting();
      });

      // Mouse Wheel event trapping.
      // Mouse wheel events are sent to the document, not the element,
      // so here we catch mouse wheel events only when the mouse goes over the element.
      // TODO: Fix Safari mouse wheel events..
      var mouseWheelActivated = false;
      var activateMousewheelFn = function(evt) {
        if(mouseWheelActivated){
          return;
        }
        var mousewheelFn = function(evt) {
          if(evt.detail) evt.wheelDelta = evt.detail * -50;
          fireEvent('mousewheel', evt);
        }
        document.addEventListener('mousewheel', mousewheelFn, false);
        document.addEventListener('DOMMouseScroll', mousewheelFn, false);
        var deactivateMousewheelFn = function(evt) {
          windowElement.removeEventListener('mouseout', deactivateMousewheelFn, false);
          document.removeEventListener('mousewheel', mousewheelFn, false);
          mouseWheelActivated = false;
        }
        windowElement.addEventListener('mouseout', deactivateMousewheelFn, false);
        mouseWheelActivated = true;
      }
      windowElement.addEventListener('mousemove', activateMousewheelFn, false);
      scene.addEventHandlingFunctions(viewportNode);
    }

    if (options.cameraNode) {
      viewportNode.pub.setCameraNode(options.cameraNode);
    }

    return viewportNode;
  }});

  
/**
 * The rendering scene graph is the core implementation of Fabric's
 * additional rendering pipeline and capabilities.
 */
FABRIC.SceneGraph.registerNodeType('OffscreenViewport', {
  briefDesc: 'The OffscreenViewport node offers framebuffer based drawing.',
  detailedDesc: 'Utilizing a custom event and eventhandler, the OffscreenViewport node offers a powerful OpenGL framebuffer, ' +
                'which custom shaders can draw to. You can use this for computing depth as well as UV channels, for example. '+
                'The OffscreenViewport node uses the same camera and window as the standard viewport.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    mainViewportNode: 'The viewport this OffscreenViewport will use the window and and viewport dgnode from.',
    cameraNode: 'An optional camera to be used for this OffscreenViewport. If not specified, the camera of the mainViewportNode is used.',
    eventName: 'The name of the event to be created. The OffscreenViewport will listen to this event to be fired for a redraw.',
    textureDescription: 'An optional parameter to define the texture definition which is used to store the OffscreenViewport\'s image.',
    clearColor: 'clear (background) color' //TODO: make this work properly by initializing colors from current ones (background texture)
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        mainViewportNode: undefined,
        cameraNode: undefined,
        eventName: undefined,
        textureDescription: undefined,
        clearColor: FABRIC.RT.rgba(0.0,0.0,0.0,0.0)
      });

    // query the main viewport
    if (!options.mainViewportNode) {
      throw ('Must provide a mainViewportNode to this constructor');
    }
    var mainViewportNode = options.mainViewportNode;
    if(!mainViewportNode.isTypeOf('Viewport')) {
      throw ('mainViewport is not a Viewport!');
    }
    
    // query the camera
    var cameraNode;
    if(options.cameraNode)
      cameraNode = options.cameraNode;
    else
      cameraNode = mainViewportNode.getCameraNode();
    if(!cameraNode.isTypeOf('Camera')) {
      throw ('cameraNode is not a Camera!');
    }
    
    // query the eventName
    if(!options.eventName) {
      throw(' Must provide an eventName to this constructor!');
    }
    var eventName = options.eventName+'';
    
    // create the offscreennode
    var offscreenNode = scene.constructNode('SceneGraphNode', {name: eventName});
    var dgnode = offscreenNode.constructDGNode('DGNode');
    
    // create the event and handler
    var eventNode = offscreenNode.constructEventNode(eventName);
    var eventHandlerNode = offscreenNode.constructEventHandlerNode('Redraw');
    eventNode.appendEventHandler(eventHandlerNode);
    
    // create the render target
    var textureDescription = options.textureDescription;
    if(!textureDescription) {
      textureDescription = new FABRIC.RT.OGLRenderTargetTextureDesc (
        2,
        new FABRIC.RT.OGLTexture2D (
          FABRIC.SceneGraph.OpenGLConstants.GL_RGBA16,
          FABRIC.SceneGraph.OpenGLConstants.GL_RGBA,
          FABRIC.SceneGraph.OpenGLConstants.GL_FLOAT)
      );
    }
    dgnode.addMember('renderTarget', 'OGLRenderTarget', FABRIC.RT.oglRenderTarget(0,0,[textureDescription]));
    
    // extend the public interface
    offscreenNode.pub.getEventName = function() {
      return eventName;
    };
    offscreenNode.pub.getEventHandler = function() {
      return eventHandlerNode;
    };
    offscreenNode.pub.redraw = function() {
      return eventNode.fire();
    };
    offscreenNode.pub.setViewportNode = function(viewport) {
      var viewport = scene.getPrivateInterface(viewport);
      if(!viewport.pub.isTypeOf('Viewport')) {
        throw ('Provided viewport is not of type Viewport!');
      }
      eventHandlerNode.setScope('viewPort',viewport.getDGNode());
      eventHandlerNode.setScope('window',viewport.getFabricWindowObject().windowNode);
    }
    offscreenNode.pub.setCameraNode = function(camera) {
      var camera = scene.getPrivateInterface(camera);
      if(!camera.pub.isTypeOf('Camera')) {
        throw ('Provided camera is not of type Camera!');
      }
      eventHandlerNode.setScope('camera',camera.getDGNode());
    }
    offscreenNode.pub.constructMaterial = function(name) {
      var materialNode = scene.constructNode(name, {
        parentEventHandler: eventHandlerNode
      });
      offscreenNode.pub['get'+name] = function() {
        return materialNode.pub;
      }
      return materialNode.pub;
    }

    // setup the known scopes
    offscreenNode.pub.setViewportNode(mainViewportNode);
    offscreenNode.pub.setCameraNode(cameraNode);
    eventHandlerNode.setScope('data',dgnode);

    // setup the prebindings in order
    var preBindings = eventHandlerNode.preDescendBindings;
    preBindings.append(
      scene.constructOperator({
          operatorName: 'bindScreenRenderTarget',
          srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
          entryPoint: 'bindScreenRenderTarget',
          parameterLayout: [
            'window.width',
            'window.height',
            'data.renderTarget'
          ]
        }));
    
    preBindings.append(scene.constructOperator({
      operatorName: 'UpdateCameraProjection',
      srcFile: 'FABRIC_ROOT/SG/KL/camera.kl',
      entryPoint: 'updateCameraProjection',
      parameterLayout: [
        'camera.projectionMat44',
        'window.width',
        'window.height',
        'camera.nearDistance',
        'camera.farDistance',
        'camera.fovY',
        'camera.orthographic',
        'camera.screenOffset'
      ]
    }));

    eventHandlerNode.postDescendBindings.insert(
      scene.constructOperator({
          operatorName: 'unbindRenderTarget',
          srcFile: 'FABRIC_ROOT/SG/KL/renderTarget.kl',
          entryPoint: 'unbindRenderTarget',
          parameterLayout: [
            'data.renderTarget'
          ]
        }), 0);
    

    return offscreenNode;
  }});

});

