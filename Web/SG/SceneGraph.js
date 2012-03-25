/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(function() {

/**
 * The scene graph is the core implementation of Fabric's
 * node graph in javascript.
 */
FABRIC.SceneGraph = {
  managerDescriptions: {},
  nodeDescriptions: {},
  assetLoaders: {},
  registerManagerType: function(type, description) {
    if (this.managerDescriptions[type]) {
      throw ('Manager Constructor already Registered:' + type);
    }else {
        // Commented out till we can finish the documentation.
  //    if (!nodeDescription.briefDesc || !nodeDescription.detailedDesc)
  //      console.log('WARNING: Node Constructor "'+type+'" does not provide a proper description.');
      if(!description.briefDesc) description.briefDesc = '';
      if(!description.detailedDesc) description.detailedDesc = '';
      if(!description.optionsDesc) description.optionsDesc = {};
      if(!description.parentNodeDesc) description.parentNodeDesc = '';
      this.managerDescriptions[type] = description;
    }
  },
  registerNodeType: function(type, description) {
    if (this.nodeDescriptions[type]) {
      throw ('Node Constructor already Registered:' + type);
    }else {
      if (!description.factoryFn)
        throw ('Node Constructor "'+type+'" does not implement the factoryFn');
        // Commented out till we can finish the documentation.
  //    if (!nodeDescription.briefDesc || !nodeDescription.detailedDesc)
  //      console.log('WARNING: Node Constructor "'+type+'" does not provide a proper description.');
      if(!description.briefDesc) description.briefDesc = '';
      if(!description.detailedDesc) description.detailedDesc = '';
      if(!description.optionsDesc) description.optionsDesc = {};
      if(!description.parentNodeDesc) description.parentNodeDesc = '';
      this.nodeDescriptions[type] = description;
    }
  },
  help: function(type) {
    var result = {};
    if (!type) {
      for (type in this.nodeDescriptions) {
        result[type] = {};
        result[type].brief = this.nodeDescriptions[type].briefDesc;
        result[type].detailed = this.nodeDescriptions[type].detailedDesc;
        result[type].parentDesc = this.nodeDescriptions[type].parentNodeDesc;
        result[type].optionsDesc = this.nodeDescriptions[type].optionsDesc;
      }
    } else {
      if (!this.nodeDescriptions[type])
        throw ('Node Constructor "'+type+'" is not registered!');
      result.brief = this.nodeDescriptions[type].briefDesc;
      result.detailed = this.nodeDescriptions[type].detailedDesc;
      if(this.nodeDescriptions[this.nodeDescriptions[type].parentNodeDesc]){
        result.parentDesc = this.help(this.nodeDescriptions[type].parentNodeDesc);
      }
      result.optionsDesc = this.nodeDescriptions[type].optionsDesc;
    }
    return result;
  },
  registerParser: function(ext, parserFn) {
    ext = ext.toLocaleLowerCase();
    if (!this.assetLoaders[ext]) {
      this.assetLoaders[ext] = function(scene, assetFile, options, callback) {
        if (!options) options = {};
        var pathArray = assetFile.split('/');
        var fileName = pathArray.pop();
        options.baseName = fileName.split('.')[0];
        options.basePath = pathArray.join('/');
        return parserFn(scene, assetFile, options, callback);
      }
    }
  },
  createScene: function(sceneOptions) {
    
    if(!sceneOptions)sceneOptions = {};
    
    var context = FABRIC.createContext({'contextID':sceneOptions.contextID});
    
    var assignDefaults = function(options, defaults, force) {
      if (!options) options = {};
      for (var i in defaults) {
        if (options[i] === undefined || force) options[i] = defaults[i];
      }
      return options;
    };
    
    sceneOptions = assignDefaults(sceneOptions, {
        constructGlobalsNode: true,
        preDraw: true,
        postDraw: true,
        constructAnimationInterface: true,
        timeStep: 1/50 /* 50 fps */
      });
    
    // first let's create the basic scene object
    // we will have a private (complete) as well as
    // a public interface, which we will return from this
    // function
    var scene = {
      pub: {
      }
    };
    
    // [pzion 20110711] This is a bit of a hack: we populate the *global*
    // OpenGL constants structure if it doesn't alrady exist.
    if (!FABRIC.SceneGraph.OpenGLConstants)
      FABRIC.SceneGraph.OpenGLConstants = JSON.parse(context.EX.getLoadedExts().FabricOGL.jsConstants);

    // EAch Viewport creates a new fabricwindow which is the origin of
    // window redraw events.
    var viewports = [];

    ///////////////////////////////////////////////////////////////////
    // Maps
    var operatorStore = {};
    var sceneGraphNodes = {};
    var shaderNodeStore = {};

    ///////////////////////////////////////////////////////////////////
    // Profiling
    var Timer = function(name) {
      this.name = name;
      this.count = 0;
      this.time = 0.0;
      this.childTimers = {};
      this.start = function() {
        this.pushtime = (new Date).getTime();
      }
      this.stop = function() {
        this.count += 1;
        var bracket = (new Date).getTime() - this.pushtime;
        this.time += bracket;
        if (!profiles[this.name]) {
          profiles[this.name] = { time: bracket, count: 1 };
        }else {
          profiles[this.name].time += bracket;
          profiles[this.name].count += 1;
        }
      }
      this.start();
    }
    var timerTree, currTimer;
    currTimer = new Timer('createScene');
    timerTree = currTimer;
    var timerStack = [];
    var profiles = {};

    var pushTimer = function(name) {
      var timer = currTimer.childTimers[name];
      if (timer) {
        timer.start();
      }else {
        timer = new Timer(name);
        currTimer.childTimers[name] = timer;
      }
      currTimer = timer;
      timerStack.push(timer);
    }
    var popTimer = function(name) {
      var timer = timerStack.pop();
      timer.stop();
      if (timerStack.length > 0) {
        currTimer = timerStack[timerStack.length - 1];
      }else {
        currTimer = timerTree;
      }
    }

    /////////////////////////////////////////////////////////////////////
    // The private scene object is passed into the constructor and can augment
    // objects with extra functionality.

    // Use the following methods with *extreme* caution. please...
    scene.getContext = function() {
      return context;
    };
    scene.pub.getContextId = function() {
      return context.getContextID();
    };
    scene.bindViewportToWindow = function(element, viewPort, options) {
      var fabricwindow = context.createWindow(element, options);
      // [pzion 20110326] Add a context menu item for any windows
      // in the context that pops up a Fabric debugger for the context
      // Note: PT 19-09-11
      // This feature has only been working on OsX, and so we are diabling
      // it for the Fabric beta to ensure the developer experience is clear
      // and consistent across platforms. Developers on OsX, feel free to
      // uncomment this code :)
    //  fabricwindow.addPopUpMenuItem(
    //    'display-core-debugger',
    //    'Fabric debugger...',
    //    function(arg) {
    //      scene.pub.displayDebugger();
    //    }
    //  );
      viewports.push(viewPort);
      return fabricwindow;
    };
    scene.getRegisteredTypesManager = function() {
      return context.RegisteredTypesManager;
    };

    ///////////////////////////////////////////////////////////////////
    //  Utility functions
    scene.assignDefaults = assignDefaults;
    
    scene.cloneObj = function(obj, assignedValues) {
      var i, clonedobj = {};
      for (i in obj) {
        clonedobj[i] = obj[i];
      }
      if(assignedValues){
        for (i in assignedValues) {
          clonedobj[i] = assignedValues[i];
        }
      }
      return clonedobj;
    };
    
    //////////////////////////////////////////////////
    // Timers.
    scene.pushTimer = function(name) {
      pushTimer(name);
    };
    scene.popTimer = function(name) {
      popTimer(name);
    };
    //////////////////////////////////////////////////
    // Constructors for the CORE
    scene.constructEventNode = function(name) {
      return context.DependencyGraph.createEvent(name);
    };
    scene.constructEventHandlerNode = function(name) {
      return context.DependencyGraph.createEventHandler(name);
    };
    scene.constructResourceLoadNode = function(name) {
      return context.DependencyGraph.createResourceLoadNode(name);
    };
    scene.constructDependencyGraphNode = function(name, isResourceLoad) {
      return context.DependencyGraph.createNode(name);
    };
    
    var managers = {};
    scene.constructManager = function(type, options) {
      if (!FABRIC.SceneGraph.managerDescriptions[type]) {
        throw ('Manager Constructor not Registered:' + type);
      }
      if (managers[type]) {
        console.warn ('Manager of this type already constructed:' + type);
      }
      options = (options ? options : {});
      var managerNode = FABRIC.SceneGraph.managerDescriptions[type].factoryFn(options, scene);
      if (!managerNode) {
        throw (' Factory function method must return an object');
      }
      managers[type] = managerNode;
      return managerNode;
    };
    scene.getManager = function( type ){
      if(managers[type]){
        return managers[type].pub;
      }
      return undefined;
    }
    
    scene.constructNode = function(type, options) {
      if (!FABRIC.SceneGraph.nodeDescriptions[type]) {
        throw ('Node Constructor not Registered:' + type);
      }
      options = (options ? options : {});
      if(!options.type) options.type = type;
      var sceneGraphNode = FABRIC.SceneGraph.nodeDescriptions[type].factoryFn(options, scene);
      if (!sceneGraphNode) {
        throw (' Factory function method must return an object');
      }
      var parentTypeOfFn = sceneGraphNode.pub.isTypeOf;
      sceneGraphNode.pub.isTypeOf = function(classname) {
        if (classname == type) {
          return true;
        }else if (parentTypeOfFn !== undefined) {
          return parentTypeOfFn(classname);
        }else {
          return false;
        }
      }
      return sceneGraphNode;
    };
    
    scene.renameNode = function(privateNode, newname) {
      if (!sceneGraphNodes[privateNode.pub.getName()]) {
        throw ('SceneGraphNode "' + privateNode.pub.getName() + '" does not exist!');
      }
      if (sceneGraphNodes[newname]) {
        throw ('Name "' + newname + '" is already taken');
      }
      delete sceneGraphNodes[privateNode.pub.getName()];
      sceneGraphNodes[newname] = privateNode;
    }
    scene.getPrivateInterface = function(publicNode) {
      if (publicNode.pub && publicNode.pub.getName) {
        return publicNode;
      }
      if (!publicNode.getName) {
        throw ('Given object is not a valid public interface.');
      }
      if (!sceneGraphNodes[publicNode.getName()]) {
        throw ('SceneGraphNode "' + publicNode.getName() + '" does not exist!');
      }
      return sceneGraphNodes[publicNode.getName()];
    };
    scene.getPublicInterface = function(privateNode) {
      if (!privateNode.pub) {
        throw ('Given object does not have a public interface.');
      }
      return privateNode.pub;
    };
    scene.constructShaderNode = function(shaderType, options) {
      if (shaderNodeStore[shaderType]) {
        return shaderNodeStore[shaderType];
      }
      var shader = this.constructNode('Shader', options);
      shaderNodeStore[shaderType] = shader;
      return shader;
    };
    scene.constructOperator = function(operatorDef) {
      
      var constructBinding = function(operator) {
        var binding = context.DG.createBinding();
        binding.setOperator(operator);
        binding.setParameterLayout(operatorDef.parameterLayout ? operatorDef.parameterLayout : []);
        return binding;
      }
      
      var uid = operatorDef.operatorName;
      if (operatorStore[uid]) {
        // If this operator has already been constructed,
        // reuse the exiting one. returne a unique binding.
        return constructBinding(operatorStore[uid]);
      }
      
      var operator = context.DG.createOperator(uid);
      var filename;
      operatorStore[uid] = operator;
      
      ///////////////////////////////////////////////////
      // Construct the operator
      var configureOperator = function(code) {
        var descDiags;
        
        if (operatorDef.mainThreadOnly){
          operator.setMainThreadOnly(true);
        }
  
        descDiags = function(fullCode, diags) {
          var fullCodeLines = fullCode.split('\n');
          var desc = 'Error compiling operator: ' + operatorDef.operatorName;
          for (var i = 0; i < diags.length; ++i) {
            desc += '\nFile:' + diags[i].filename + '\n';
            if (i == 16) {
              desc += '(' + (diags.length - i) + ' more diagnostic(s) omitted)\n';
              break;
            }
            desc += diags[i].line + ':' + diags[i].column + ': ' + diags[i].level + ': ' + diags[i].desc + '\n';
            var line = diags[i].line - 1;
            for (var j = line - 6; j <= line + 6; ++j) {
              if (j >= 0 && j < fullCodeLines.length) {
                if (j == line)
                  desc += '>>>\t';
                else
                  desc += '\t';
                desc += '' + fullCodeLines[j] + '\n';
              }
            }
          }
          return desc;
        };
  
        operator.setEntryPoint(operatorDef.entryPoint);
        try {
          operator.setSourceCode(filename, code);
        }
        catch (e) {
          var message = 'Error compiling operator: ' + operatorDef.operatorName + '\n';
          if (operatorDef.srcFile) message += ' File:' + operatorDef.srcFile + '\n';
          else message += ' Code:' + operatorDef.srcCode + '\n';
          throw (message + e);
        }
        var diagnostics = operator.getDiagnostics();
        if (diagnostics.length > 0) {
          delete operatorStore[uid];
          console.error(descDiags(operator.getSourceCode(), diagnostics));
        }
      }

      if (operatorDef.srcFile) {
        filename = operatorDef.srcFile;
        if(operatorDef.async === false){
          var code = FABRIC.loadResourceURL(operatorDef.srcFile, 'text/plain');
          code = FABRIC.preProcessCode(code, operatorDef.preProcessorDefinitions);
          configureOperator(code);
        }else{
          FABRIC.loadResourceURL(operatorDef.srcFile, 'text/plain', function(code){
            code = FABRIC.preProcessCode(code, operatorDef.preProcessorDefinitions);
            configureOperator(code);
          });
        }
      }
      else if (operatorDef.srcCode) {
        filename = operatorDef.operatorName;
        // Fake an asynchronous operator construction so that we don't block waiting
        // for the operator compilation.
        if(operatorDef.async === false){
          var code = FABRIC.preProcessCode(operatorDef.srcCode, operatorDef.preProcessorDefinitions);
          configureOperator(code);
        }else{
          FABRIC.createAsyncTask(function(){
            var code = FABRIC.preProcessCode(operatorDef.srcCode, operatorDef.preProcessorDefinitions);
            configureOperator(code);
          });
        }
      }else{
        throw "Invalid operator definition. Either a source file must be specified, or source code.";
      }
      return constructBinding(operator);
    };
    //////////////////////////////////////////////////////////
    // Accessors
    var globalsNode;
    scene.getGlobalsNode = function() {
      return globalsNode;
    };
    scene.getScenePreRedrawEventHandler = function() {
      return preDrawEventHandler;
    };
    scene.getSceneRedrawEventHandler = function() {
      return beginDrawEventHandler;
    };
    scene.getSceneRedrawOpaqueObjectsEventHandler = function() {
      return beginDrawOpaqueObjectsEventHandler;
    };
    scene.getSceneRedrawTransparentObjectsEventHandler = function() {
      return beginDrawTransparentObjectsEventHandler;
    };
    scene.getSceneRedrawOverlayObjectsEventHandler = function() {
      return beginDrawOverlayObjectsEventHandler;
    };
    
    scene.getScenePostRedrawEventHandler = function() {
      return postDrawEventHandler;
    };
    scene.getSceneRaycastEventHandler = function() {
      return sceneRaycastEventHandler;
    };
    //////////////////////////////////////////////////////////
    // Shadow Mapping
    scene.registerShadowCastingLightSourceHandler = function(lightSourceRenderhandler) {
      propagateRenderShadowMapEvent.appendChildEventHandler(lightSourceRenderhandler);
      lightSourceRenderhandler.appendChildEventHandler(beginRenderShadowMap);
    };
    scene.getBeginRenderShadowMapEventHandler = function() {
      return beginRenderShadowMap;
    };
    scene.addEventHandlingFunctions = function(obj) {
      // We store a map of arrays of event listener functions.
      var eventListeners = {};
      obj.pub.addEventListener = function(type, fn) {
        if(!fn) throw "Listener Function not provided";

        if(pendingEventRemovals[type]) {
          pendingEventRemovals[type].push(fn);
        }
        else {
          if (!eventListeners[type]) {
            eventListeners[type] = [];
          }
          eventListeners[type].push(fn);
        }
      };
      var pendingEventRemovals = {};//checkRemove: allow removeEventListener() while fireEvent()

      obj.pub.removeEventListener = function(type, fn) {
        if(!fn) throw "Listener Function not provided";
        if (eventListeners[type]) {
          var id = eventListeners[type].indexOf(fn);
          if (id !== -1) {
            eventListeners[type].splice(id, 1);
          }
        }
      
      };
      obj.pub.fireEvent = function(type, evt) {

        var pendingEventRemovals = [];

        for (i in eventListeners[type]) {
          if (eventListeners[type].hasOwnProperty(i)) {
            var ret = eventListeners[type][i].call(this, evt);
            if( ret === 'remove' )
              pendingEventRemovals.push(eventListeners[type][i]);
          }
        }

        for (i in pendingEventRemovals) {
          obj.pub.removeEventListener(type,pendingEventRemovals[i]);
        }
      }
      return obj;
    };

    /////////////////////////////////////////////////////////////////////

    scene.pub.displayDebugger = function() {
      FABRIC.displayDebugger(context);
    };
    scene.getSceneGraphNodes = function(name) {
      return sceneGraphNodes;
    };
    scene.pub.getSceneGraphNode = function(name) {
      if(sceneGraphNodes[name]){
        return sceneGraphNodes[name].pub;
      }
      return undefined;
    };
    scene.setSceneGraphNode = function(name,node) {
      return sceneGraphNodes[name] = node;
    };
    scene.pub.constructManager = function(type, options) {
      return scene.constructManager(type, options).pub;
    };
    scene.pub.constructNode = function(type, options) {
      return scene.constructNode(type, options).pub;
    };
    // TODO: Remove the following 2 functoins and test.
    scene.pub.readResourceFile = function(filepath) {
      return scene.readResourceFile(filepath);
    };
    scene.pub.writeResourceFile = function(filepath, fileContents) {
      scene.writeResourceFile(filepath, fileContents);
    };
    scene.pub.getGlobalsNode = function() {
      return globalsNode;
    };
    scene.pub.importAssetFile = function(file, options, callback) {
      var ext = file.split('.').pop().toLocaleLowerCase();
      if(file.substr(0,11).toLocaleLowerCase() == "fabricio://") {
        var filename = FABRIC.IO.getFileHandleInfo(file).fileName;
        ext = filename.split('.').pop().toLocaleLowerCase();
        options.storeDataAsFile = true;
      }
      if (FABRIC.SceneGraph.assetLoaders[ext]) {
        var assets = FABRIC.SceneGraph.assetLoaders[ext](scene.pub, file, options, callback);
        return assets;
      }
      else {
        throw ('Missing Resource Loader for :' + ext);
      }
    };
    scene.pub.redrawAllViewports = function(force) {
      for (var i=0; i<viewports.length; i++) {
        viewports[i].pub.redraw(force);
      }
    };
    scene.pub.getErrors = function() {
      var errors = [];
      var allNamedObjects = context.DG.getAllNamedObjects();
      for (i in allNamedObjects) {
        if (allNamedObjects.hasOwnProperty(i)) {
          var dgErrors = allNamedObjects[i].getErrors();
          if(!dgErrors){
            console.warn( i + " does not return an errors array");
            continue;
          }
          if (dgErrors.length > 0) {
            errors.push(i + ':' + JSON.stringify(dgErrors));
          }
        }
      }
      return errors;
    };
    
    scene.addEventHandlingFunctions(scene);
    
    window.addEventListener('unload', function(){
      scene.pub.fireEvent('unloading');
    }, false);

    // and the shaders will be left connected to this node. Multiple
    // cameras can render the scene by connecting to this node.
    var preDrawEventHandler, postDrawEventHandler;
    var propagateRenderShadowMapEvent, beginRenderShadowMap, shadowMapMaterial;
    if(sceneOptions.preDraw){
      preDrawEventHandler = scene.constructEventHandlerNode('Scene_PreDraw');
      propagateRenderShadowMapEvent = scene.constructEventHandlerNode('PropagateRenderShadowMapEvent');
      
      ///////////////////////////////////////////////////////////////////
      // Shadowcasting Lightsource <-> SceneGraph draw event handler firewall
      beginRenderShadowMap = scene.constructEventHandlerNode('BeginRenderShadowMap');
      
        
      // The 'Parent' node is the first child of the scene redraw event.
      // This means that the event traversial will propagate down the
      // shadow casting graph, before propagating down the render graph.
      preDrawEventHandler.appendChildEventHandler(propagateRenderShadowMapEvent);
    }
    if(sceneOptions.postDraw){
      postDrawEventHandler = scene.constructEventHandlerNode('Scene_PostDraw');
    }

    ///////////////////////////////////////////////////////////////////
    // All scene draw event handlers(shaders) are attached to this handler.
    var beginDrawEventHandler = scene.constructEventHandlerNode('Scene_Draw');
    
    var beginDrawOpaqueObjectsEventHandler = scene.constructEventHandlerNode('Scene_DrawOpaqueObjects');
    beginDrawEventHandler.appendChildEventHandler(beginDrawOpaqueObjectsEventHandler);
    
    // Transparent objects are always drawn after opaque objects
    var beginDrawTransparentObjectsEventHandler = scene.constructEventHandlerNode('Scene_DrawTransparentObjects');
    beginDrawEventHandler.appendChildEventHandler(beginDrawTransparentObjectsEventHandler);
    
    // Overlay objects are always drawn after everything else
    var beginDrawOverlayObjectsEventHandler = scene.constructEventHandlerNode('Scene_DrawOverlaybjects');
    beginDrawEventHandler.appendChildEventHandler(beginDrawOverlayObjectsEventHandler);
    
    ///////////////////////////////////////////////////////////////////
    // Window <-> SceneGraph raycast event handler firewall
    var sceneRaycastEventHandler = scene.constructEventHandlerNode('Scene_raycast');

    if (sceneOptions.constructGlobalsNode) {
      globalsNode = scene.constructDependencyGraphNode('Scene_globals');

      if (sceneOptions.constructAnimationInterface) {
        // All time values are in seconds. 
        globalsNode.addMember('time', 'Scalar', 0);
        globalsNode.addMember('time_prevupdate', 'Scalar', 0);
        globalsNode.addMember('timestep', 'Scalar', 0);
        globalsNode.bindings.append(scene.constructOperator( {
          operatorName:'calcTimeStep',
          srcCode:'operator calcTimeStep(io Scalar t, io Scalar prev_t, io Scalar delta_t){ delta_t = t - prev_t; prev_t = t; }',
          entryPoint:'calcTimeStep',
          parameterLayout: [
            'self.time',
            'self.time_prevupdate',
            'self.timestep'
          ]
        }));
        
        var isPlaying = false, time = 0;
        var setTime = function(t, redraw) {
          time = Math.round(t/sceneOptions.timeStep) * sceneOptions.timeStep;
          globalsNode.setData('time', 0, time);
          scene.pub.fireEvent('timechanged', { time: time, playing: isPlaying });
          if(redraw !== false){
            scene.pub.redrawAllViewports(true);
          }
        }
        var timeStepMS = sceneOptions.timeStep * 1000.0;
        var prevTime, prevFrameDuration = 0;
      //  var frameStartTime, frameRate = 0;
        var advanceTime = function() {
          var t = time + sceneOptions.timeStep;
          
          // The computer will attempt to play back
          // at exactly the given frame rate. If the
          // frame rate cannot be achieved it plays 
          // as fast as possible. The time step as
          // used throughout the graph will always 
          // be fixed at the given rate.
          var currTime = (new Date).getTime();
          var prevFrameDuration = (currTime - prevTime);
          
          // Measuring the frame time using JavaScript gives
          // garbage results, and I'm not sure why. 
        //  frameRate = (currTime - frameStartTime);
        //  console.log("frameRate:"+frameRate);
        //  frameStartTime = currTime;
          if(prevFrameDuration < timeStepMS){
            var delay = (timeStepMS - prevFrameDuration);
            setTimeout(function(){
                prevTime = currTime + delay;
                setTime(t);
              },
              delay
            );
          }else{
            prevTime = currTime;
            setTime(t);
          }
        }
        /////////////////////////////////////////////////////////
        // Animation Interface
        scene.pub.animation = {
          setTime:function(t, redraw) {
            setTime(t, redraw);
          },
          getTime:function() {
            return time;
          },
          getTimeStep:function() {
            return sceneOptions.timeStep;
          },
          play: function() {
            prevTime = (new Date).getTime();
            isPlaying = true;
            // Note: this is a big ugly hack to work around the fact that
            // we have zero or more windows. What happens when we have
            // multiple viewports? Should the 'play' controls be moved to
            // Viewport?
            prevTime = (new Date).getTime();
            scene.getContext().VP.viewPort.setRedrawFinishedCallback(advanceTime);
            scene.getContext().VP.viewPort.needsRedraw();
          },
          pause: function() {
            isPlaying = false;
            scene.getContext().VP.viewPort.setRedrawFinishedCallback(null);
          },
          reset: function() {
            isPlaying = false;
            scene.getContext().VP.viewPort.setRedrawFinishedCallback(null);
            setTime(0, true);
            globalsNode.setData('time_prevupdate', 0, 0);
          },
          step: function() {
            advanceTime();
          }
        };
        scene.isPlaying = function(){
          return isPlaying;
        }
      }
    }
    
    var loadStartTime = (new Date()).getTime();
    FABRIC.appendOnResolveAsyncTaskCallback(function(label, nbRemaining, doneWeight, totalWeight) {
      if (nbRemaining===0) {
        scene.pub.fireEvent('sceneloaded', {
          loadTime: ((new Date()).getTime() - loadStartTime)
        });
        return true;
      }
      return false;
    });

    // only return the public interface to the outside world
    return scene.pub;
  }
};

FABRIC.SceneGraph.registerNodeType('SceneGraphNode', {
  briefDesc: 'The base class for the SceneGraphNodes.',
  detailedDesc: 'The SceneGraphNode is a basic wrapper for Fabric\'s DGNode. Each SceneGraphNode can '+
                'contain several DGNodes. Furthermore the SceneGraphNode implements the concept of a '+
                'private and public interface. All methods which are public (outside of the scope of) '+
                'a constructor) are added to the "pub" member of the SceneGraphNode, whereas private '+
                'methods (inside of the scope of a constructor) are attached to the SceneGraphNode '+
                'object itself.',
  factoryFn: function(options, scene) {

    var dgnodes = {};
    var eventnodes = {};
    var eventhandlernodes = {};
    var memberInterfaces = {};
    var nodeReferenceInterfaces = {};
    var nodeReferences = {};

    var capitalizeFirstLetter = function(str) {
      return str[0].toUpperCase() + str.substr(1);
    };
    
    // ensure the name is unique
    var name, type = options.type;
    
    var sceneGraphNode = {
      pub: {
        getName: function() {
          return name;
        },
        setName: function(newname) {
          if (scene.pub.getSceneGraphNode(newname)) {
            var prefix = 1;
            while (scene.pub.getSceneGraphNode(newname + prefix)) {
              prefix++;
            }
            newname = newname + prefix;
          }
          if(name){
            scene.renameNode(sceneGraphNode, newname);
          }
          name = newname;
        },
        getType: function() {
          return type;
        }
      },
      addMemberInterface : function(corenode, memberName, defineSetter, setterCallback) {
        var getterName = 'get' + capitalizeFirstLetter(memberName);
        var getterFn = function(sliceIndex){
          return corenode.getData(memberName, sliceIndex);
        }
        sceneGraphNode.pub[getterName] = getterFn;
        if(defineSetter===true){
          var setterName = 'set' + capitalizeFirstLetter(memberName);
          var setterFn = function(value, sliceIndex){
            corenode.setData(memberName, sliceIndex?sliceIndex:0, value);
            if(setterCallback){
              setterCallback(value);
            }
          }
          sceneGraphNode.pub[setterName] = setterFn;
          memberInterfaces[memberName] = { getterFn:getterFn, setterFn:setterFn };
        }
      },
      addReferenceInterface : function(referenceName, typeConstraint, setterCallback) {
        var getterName = 'get' + capitalizeFirstLetter(referenceName) + 'Node';
        var setterName = 'set' + capitalizeFirstLetter(referenceName) + 'Node';
        var getterFn = function(){
          return nodeReferences[referenceName];
        }
        var typeConstraints = typeConstraint.split('|');
        var setterFn = function(node, option){
          if (node){
            var matchesType = false;
            for(var i=0; !matchesType && i<typeConstraints.length; i++){
              if(node.isTypeOf(typeConstraints[i])) {
                matchesType = true;
              }
            }
            if(!matchesType){
              throw ('Incorrect type assignment. Must assign a '+typeConstraint);
            }
          }
          var prevnode = nodeReferences[referenceName];
          nodeReferences[referenceName] = node;
          // a reference can be removed by passing in undefined for the value.
          // the setterFn must handle undefined as a value.
          setterCallback(node ? scene.getPrivateInterface(node) : undefined, option);
          
          scene.pub.fireEvent('referenceassigned', {
            sgnode: sceneGraphNode.pub,
            newnode: node,
            prevnode: prevnode,
            getterFn: getterFn,
            setterFn: setterFn
          });
          return sceneGraphNode.pub;
        }
        sceneGraphNode.pub[getterName] = getterFn;
        sceneGraphNode.pub[setterName] = setterFn;
        nodeReferenceInterfaces[referenceName] = { getterFn:getterFn, setterFn:setterFn };
        return sceneGraphNode.pub[setterName];
      },
      addReferenceListInterface : function(referenceName, typeConstraint, adderCallback, removeCallback) {
        nodeReferences[referenceName] = [];
        var getterName = 'get' + capitalizeFirstLetter(referenceName) + 'Node';
        var adderName = 'add' + capitalizeFirstLetter(referenceName) + 'Node';
        var removerName = 'remove' + capitalizeFirstLetter(referenceName) + 'Node';
        var getterFn = function(index){
          return nodeReferences[referenceName][index ? index : 0];
        }
        var typeConstraints = typeConstraint.split('|');
        var adderFn = function(node){
          if (node){
            for(var i=0; i<typeConstraints.length; i++){
              if(!node.isTypeOf(typeConstraints[i])) {
                throw ('Incorrect type assignment. Must assign a '+typeConstraint);
              }
            }
          }
          var index = nodeReferences[referenceName].indexOf(node);
          if(index !== -1) return sceneGraphNode.pub;
          nodeReferences[referenceName].push(node);
          adderCallback(scene.getPrivateInterface(node));
          
          scene.pub.fireEvent('referenceadded', {
            sgnode: sceneGraphNode.pub,
            newnode: node,
            getterFn: getterFn,
            adderFn: adderFn,
            removerFn: removerFn
          });
          return sceneGraphNode.pub;
        }
        var removerFn = function(val){
          var index = -1;
          if(typeof val == 'number'){
            index = val;
          }else{
            index = nodeReferences[referenceName].indexOf(val);
          }
          if (index === -1) {
            throw ( typeConstraint + ' not assigned');
          }
          var node = nodeReferences[referenceName][index];
          removeCallback(scene.getPrivateInterface(node), index);
          nodeReferences[referenceName].splice(index, 1);
          
          scene.pub.fireEvent('referenceremoved', {
            sgnode: sceneGraphNode.pub,
            prevnode: node,
            getterFn: getterFn,
            adderFn: adderFn,
            removerFn: removerFn
          });
          return sceneGraphNode.pub;
        }
        sceneGraphNode.pub[getterName] = getterFn;
        sceneGraphNode.pub[adderName] = adderFn;
        sceneGraphNode.pub[removerName] = removerFn;
        nodeReferenceInterfaces[referenceName] = { getterFn:getterFn, adderFn:adderFn, removerFn:removerFn };
        return sceneGraphNode.pub[adderName];
      },
      constructDGNode: function(dgnodename, isResourceLoad) {
        if(dgnodes[dgnodename]){
          throw "SceneGraphNode already has a " + dgnodename;
        }
        var dgnode;
        if(isResourceLoad){
          dgnode = scene.constructResourceLoadNode(name + '_' + dgnodename);
        }
        else{
          dgnode = scene.constructDependencyGraphNode(name + '_' + dgnodename);
        }
        dgnode.sceneGraphNode = sceneGraphNode;
        sceneGraphNode['get' + dgnodename] = function() {
          return dgnode;
        };
        dgnodes[dgnodename] = dgnode;
        return dgnode;
      },
      constructResourceLoadNode: function(dgnodename) {
        return sceneGraphNode.constructDGNode(dgnodename, true);
      },
      getDGNodes: function() {
        return dgnodes;
      },
      constructEventHandlerNode: function(ehname) {
        var eventhandlernode = scene.constructEventHandlerNode(name + '_' + ehname);
        eventhandlernode.sceneGraphNode = sceneGraphNode;
        sceneGraphNode['get' + ehname + 'EventHandler'] = function() {
          return eventhandlernode;
        };
        eventhandlernodes[ehname] = eventhandlernode;
        return eventhandlernode;
      },
      constructEventNode: function(eventname) {
        eventnode = scene.constructEventNode(name + '_' + eventname);
        eventnode.sceneGraphNode = sceneGraphNode;
        eventnodes[eventname] = eventnode;
        return eventnode;
      },
      addDependencies: function(sceneSerializer) {
        for(var referenceName in nodeReferences){
          if(nodeReferences[referenceName].constructor.name == 'Array'){
            for(var i=0; i<nodeReferences[referenceName].length; i++){
              sceneSerializer.addNode(nodeReferences[referenceName][i]);
            }
          }
          else{
            sceneSerializer.addNode(nodeReferences[referenceName]);
          } 
        }
      },
      writeData: function(sceneSerializer, constructionOptions, nodeData) {
        constructionOptions.name = name;
        
        for(var referenceName in nodeReferences){
          if(nodeReferences[referenceName].constructor.name == 'Array'){
            nodeData[referenceName] =  [];
            for(var i=0; i<nodeReferences[referenceName].length; i++){
              nodeData[referenceName].push(nodeReferences[referenceName][i].getName());
            }
          }
          else {
            nodeData[referenceName] = nodeReferences[referenceName].getName();
          }
        }
        for(var memberName in memberInterfaces){
          if(!nodeData[memberName]){
            nodeData[memberName] = memberInterfaces[memberName].getterFn();
          }
        }
      },
      readData: function(sceneDeserializer, nodeData) {
        for(var referenceName in nodeReferences){
          if(nodeData[referenceName]){
            if(nodeData[referenceName].constructor.name == 'Array'){
              for(var i=0; i<nodeReferences[referenceName].length; i++){
                var dgnode = sceneDeserializer.getNode(nodeData[referenceName][i]);
                if(dgnode){
                  nodeReferenceInterfaces[referenceName].adderFn(dgnode);
                }
              }
            }
            else if(typeof nodeData[referenceName] == 'string'){
              var dgnode = sceneDeserializer.getNode(nodeData[referenceName]);
              if(dgnode){
                nodeReferenceInterfaces[referenceName].setterFn(dgnode);
              }
            }
          }
        }
        for(var memberName in memberInterfaces){
          if(nodeData[memberName]){
            memberInterfaces[memberName].setterFn(nodeData[memberName]);
          }
        }
      }
    }
    
    sceneGraphNode.pub.setName(options.name ? options.name : options.type);

    // store it to the map
    scene.setSceneGraphNode(name, sceneGraphNode);
    
    return sceneGraphNode;
  }});


FABRIC.SceneGraph.registerNodeType('ResourceLoad', {
  briefDesc: 'The ResourceLoad node implements the loading of a resource from an URL.',
  detailedDesc: 'Based on is \'url\' member, the ResourceLoad node will asynchronously load the associated ' +
                'resource to its \'resource\' member. Until the data is loaded, resource.dataSize will be zero. ' +
                'Once the data is loaded, events will be fired; you can register those by calling the ' +
                '\'addEventListener(\'loadSuccess\', func)\' and \'addEventListener(\'loadFailure\', func)\' member function. Unless ' + 
                '\'option.redrawOnLoad\' is set to false, the loading will automatically trigger a redraw. ' +
                'Note that operators can dynamically modify the URL.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    blockRedrawingTillResourceIsLoaded: 'If set to true redrawing will be blocked until the resource is loaded.',
    redrawOnLoad: 'If set to true, the viewport will fire a redraw once the resource has been loaded.',
    storeDataAsFile: 'If set to true, the resource data will be stored as a file, with handle = resource.dataExternalLocation'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      blockRedrawingTillResourceIsLoaded: true,
      redrawOnLoad: true,
      storeDataAsFile: true,
      url: undefined
    });
    
    var fileName = options.url.split('/').pop();
    var baseName = fileName.split('.')[0];
    if(!options.name){
      options.name = baseName;
    }

    var lastLoadEventURL = '';
    var lastLoadSucceeded = false;
    var url;

    var resourceLoadNode = scene.constructNode('SceneGraphNode', options);
    scene.addEventHandlingFunctions(resourceLoadNode);

    var dgnode;
      //TODO: this is probably obselete with the new storeDataAsFile option???
    {
    dgnode = resourceLoadNode.constructResourceLoadNode('DGLoadNode');
    if( options.storeDataAsFile === true )
      dgnode.setData( 'storeDataAsFile', 0, true );
    }

    resourceLoadNode.addMemberInterface(dgnode, 'url');
    
    var remainingTaskWeight = 1.0;
    var incrementLoadProgressBar;
    if(options.blockRedrawingTillResourceIsLoaded && options.url && !options.localPath){
      incrementLoadProgressBar = FABRIC.addAsyncTask("Loading: "+ options.url, remainingTaskWeight);
    }
    
    var onCommonEndLoad = function() {
      if(incrementLoadProgressBar){
        incrementLoadProgressBar(true, -remainingTaskWeight);
        incrementLoadProgressBar = undefined;
      }
      else if (options.redrawOnLoad) {
        scene.pub.redrawAllViewports();
      }
    }
    
    var onLoadSuccessCallbackFunction = function(node) {
      lastLoadSucceeded = true;
      lastLoadEventURL = resourceLoadNode.pub.getUrl();
      resourceLoadNode.pub.fireEvent('loadSuccess', resourceLoadNode.pub);
      onCommonEndLoad();
    }
    
    var onLoadProgressCallbackFunction = function(node, progress) {
      resourceLoadNode.pub.fireEvent('onProgress',{'total':progress.total, 'done':progress.received, 'node':resourceLoadNode.pub});
      prevRemainingTaskWeight = remainingTaskWeight;
      //TaskWeight = 1 + size/100KB
      remainingTaskWeight = 1.0 + (progress.total - progress.received) / 100000;
      if(incrementLoadProgressBar)
        incrementLoadProgressBar(false, remainingTaskWeight-prevRemainingTaskWeight);
    }
    
    var onLoadFailureCallbackFunction = function(node) {
      lastLoadSucceeded = false;
      lastLoadEventURL = resourceLoadNode.pub.getUrl();
      resourceLoadNode.pub.fireEvent('loadFailure', resourceLoadNode.pub);
      onCommonEndLoad();
    }

    dgnode.addOnLoadSuccessCallback(onLoadSuccessCallbackFunction);
    dgnode.addOnLoadProgressCallback(onLoadProgressCallbackFunction);
    dgnode.addOnLoadFailureCallback(onLoadFailureCallbackFunction);

    resourceLoadNode.pub.isLoaded = function() {
      return lastLoadEventURL !== '' && lastLoadEventURL === resourceLoadNode.pub.getUrl();
    }

    //This is particular to resourceLoadNode: we want to trigger the load/failure event
    //when a client registers and the resource has loaded already. This is because sometimes
    //the load is almost instantaneous and the clients usually don't cover that scenario.

    resourceLoadNode.pub.setUrl = function(new_url, forceLoad) {
      if(new_url !== '' && new_url !== url && incrementLoadProgressBar === undefined && options.blockRedrawingTillResourceIsLoaded){
        incrementLoadProgressBar = FABRIC.addAsyncTask("Loading: "+ new_url, remainingTaskWeight);
      }
      dgnode.setData('url', 0, new_url);
      url = new_url;
      if(forceLoad != false){
        dgnode.evaluate();
      }
    };

    var parentWriteData = resourceLoadNode.writeData;
    resourceLoadNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      constructionOptions.url = options.url;
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
    };
    
    scene.addEventHandlingFunctions(resourceLoadNode);

    var addEventListener = resourceLoadNode.pub.addEventListener;
    resourceLoadNode.pub.addEventListener = function(type, fn) {
      if(type === 'loadSuccess' && resourceLoadNode.pub.isLoaded() && lastLoadSucceeded)
        fn(resourceLoadNode.pub);
      else if(type === 'loadFailure' && resourceLoadNode.pub.isLoaded() && !lastLoadSucceeded)
        fn(resourceLoadNode.pub);
      else {
        addEventListener(type, fn);
      }
    }

    if (options.url) {
      // check if the url has a handle
      if(options.url.folderHandle) {
        dgnode.getResourceFromFile('resource', options.url);
      } else {
        resourceLoadNode.pub.setUrl(options.url);
      }
    }

    return resourceLoadNode;
  }});

  return FABRIC.SceneGraph;
});
