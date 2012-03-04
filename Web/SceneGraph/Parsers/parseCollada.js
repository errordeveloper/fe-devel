
//
// Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
//

FABRIC.SceneGraph.registerParser('dae', function(scene, assetFile, options, callback) {
  
  if(options.constructMaterialNodes == undefined) options.constructMaterialNodes = false;
  if(options.scaleFactor == undefined) options.scaleFactor = 1.0;
  if(options.logWarnings == undefined) options.logWarnings = false;
  if(options.constructScene == undefined) options.constructScene = true;
  
  // Load animations in the collada file into an animation library using an existing rig.
  if(options.loadAnimationUsingRig == undefined) options.loadAnimationUsingRig = false;
  if(options.loadPoseOntoRig == undefined) options.loadPoseOntoRig = false;
  if(options.constructRigFromHierarchy == undefined) options.constructRigFromHierarchy = false;
  
  // options.rigNode;
  // options.rigHierarchyRootNodeName;
  if(options.flipUVs == undefined) options.flipUVs = true;
  var animationLibrary = options.animationLibrary;
  var controllerNode = options.controllerNode;
  
  

  var assetNodes = {};
  var warn = function( warningText ){
  //  console.warn(warningText);
  }
  
    
  var makeRT = (function() {
    var ctor;
    function RT(args) {
      return ctor.apply(this, args);
    }
    return function() {
      ctor = arguments[0];
      var rt = new RT(arguments[1]);
      rt.__proto__ = ctor.prototype;
      return rt;
    }
  })();
    
    
  //////////////////////////////////////////////////////////////////////////////
  // Collada File Parsing Functions

  var parseLibaryEffects = function(node) {
  }
  
  var parseLibaryMaterials = function(node) {
  }
  
  var parseAccessor = function(node){
    var accessor = {
      source: node.getAttribute('source'),
      count: parseInt(node.getAttribute('count')),
      stride: parseInt(node.getAttribute('stride')),
      params: []
    };
                
    var technique_common = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'param':
          accessor.params.push({
            name: child.getAttribute('name'),
            type: child.getAttribute('type')
          });
          break;
        default:
          warn("Warning in parseAccessor: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return accessor;
  }
  
  var parseTechniqueCommon = function(node){
    var technique_common = { };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'accessor':
          technique_common.accessor = parseAccessor(child);
          break;
        default:
          warn("Warning in parseTechniqueCommon: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return technique_common;
  }
  
  var parseSource = function(node){
    var val, source = {
      data: [],
      technique: undefined
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'float_array':
          var count = parseInt(child.getAttribute('count'));
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              source.data.push(parseFloat(text_array[i]));
            }
          }
          if(source.data.length != count){
            throw 'Error splitting source data array';
          }
          break;
        case 'Name_array':
          var count = parseInt(child.getAttribute('count'));
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              source.data.push(text_array[i]);
            }
          }
          if(source.data.length != count){
            throw 'Error splitting source data array';
          }
          break;
        case 'IDREF_array':
          var count = parseInt(child.getAttribute('count'));
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              source.data.push(text_array[i]);
            }
          }
          if(source.data.length != count){
            throw 'Error splitting source data array';
          }
          break;
        case 'technique_common':
          source.technique = parseTechniqueCommon(child);
          break;
        default:
          warn("Warning in parseSource: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return source;
  }
  
  var parseInput = function(node){
    var input = {
      'semantic': node.getAttribute('semantic'),
      'source': node.getAttribute('source')
    };
    if(node.getAttribute('offset')){
      input.offset = parseInt(node.getAttribute('offset'));
    }
    if(node.getAttribute('set')){
      input.set = parseInt(node.getAttribute('set'));
    }
    return input;
  }
  
  var parseSampler = function(node) {
    var inputs = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'input':
          inputs[child.getAttribute('semantic')] = parseInput(child);
          break;
        default:
          warn("Warning in parseLibaryGeometries: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return inputs;
  }
  
  var parseAnimation = function(node, channelMap) {
    var animationData = {
      sources: {}
      };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'source':
          animationData.sources[child.getAttribute('id')] = parseSource(child);
          break;
        case 'sampler':
          animationData.sampler = parseSampler(child);
          break;
        case 'channel':
          animationData.channel = {
            source: child.getAttribute('source'),
            target: child.getAttribute('target')
          }
          break;
        default:
          warn("Warning in parseAnimation: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    var targetNodeName = animationData.channel.target.split('/')[0];
    var targetChannelName = animationData.channel.target.split('/')[1];
    
    if(!channelMap[targetNodeName]){
      channelMap[targetNodeName] = {};
    }
    channelMap[targetNodeName][targetChannelName] = animationData;
    
    return animationData;
  }
  
  var parseLibaryAnimations = function(node) {
    var libraryAnimations = {
      animations: [], /* array of all animation channels */
      channelMap: {} /* mapping of animation tracks to channels */
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'animation':
          // Note: collada files exported from 3dsmax have an extra level here
          // where animation is nested under animation. it also has a 'name'
          // attribute for the name of the target node.
          var animationNode = child;
          if(animationNode.firstElementChild.nodeName == 'animation'){
            animationNode = animationNode.firstElementChild;
          }
          libraryAnimations.animations[child.getAttribute('id')] = parseAnimation(animationNode, libraryAnimations.channelMap);
          break;
        default:
          warn("Warning in parseLibaryGeometries: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return libraryAnimations;
  }
  
  var parsePolygons = function(node){
    var polygons = {
      material: node.getAttribute('material'),
      count: parseInt(node.getAttribute('count')),
      inputs: {},
      indices: []
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'input':
          polygons.inputs[child.getAttribute('semantic')] = parseInput(child);
          break;
        case 'p':
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              polygons.indices.push(parseFloat(text_array[i]));
            }
          }
          break;
        default:
          warn("Warning in parsePolygons: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return polygons;
  }
  
  var parseMesh = function(node){
    var mesh = {
      sources: {},
      vertices: {}
    };
    
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'source':
          mesh.sources[child.getAttribute('id')] = parseSource(child);
          break;
        case 'vertices':
          mesh.vertices = parseInput(child.firstElementChild);
          break;
        case 'polygons':
          if(!mesh.polygons) mesh.polygons = [];
          mesh.polygons.push(parsePolygons(child));
          break;
        case 'triangles':
          if(!mesh.triangles) mesh.triangles = [];
          mesh.triangles.push(parsePolygons(child));
          break;
        default:
          warn("Warning in parseMesh: Unhandled node '" + child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return mesh;
  }
  
  var parseGeometry = function(node){
    var geometry = {
      name: node.getAttribute('name')
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'mesh':
          geometry.mesh = parseMesh(child);
          break;
        default:
          warn("Warning in parseGeometry: Unhandled node '" + child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return geometry;
  }
  
  var parseLibaryGeometries = function(node) {
    var libraryGeometries = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'geometry':
          libraryGeometries[child.getAttribute('id')] = parseGeometry(child);
          break;
        default:
          warn("Warning in parseLibaryGeometries: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return libraryGeometries;
  }
  
  var parseVertexWeights = function(node) {
    var vertexWeights = {
      count: parseInt(node.getAttribute('count')),
      inputs: {},
      vcounts: [],
      indices: []
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'input':
          vertexWeights.inputs[child.getAttribute('semantic')] = parseInput(child);
          break;
        case 'vcount':
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              vertexWeights.vcounts.push(parseInt(text_array[i]));
            }
          }
          break;
        case 'v':
          var text_array = child.textContent.split(new RegExp("\\s+"));
          for(var i=0; i<text_array.length; i++){
            if(text_array[i] != ""){
              vertexWeights.indices.push(parseInt(text_array[i]));
            }
          }
          break;
        default:
          warn("Warning in parseVertexWeights: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return vertexWeights;
  }
  
  var parseJoints = function(node) {
    var joints = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'input':
          joints[child.getAttribute('semantic')] = parseInput(child);
          break;
        default:
          warn("Warning in parseJoints: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return joints;
  }
  
  var parseMatrix = function(node) {
    var text_array = node.textContent.split(new RegExp("\\s+"));
    var float_array = [];
    for(var i=0; i<text_array.length; i++){
      if(text_array[i] != ""){
        float_array.push(parseFloat(text_array[i]));
      }
    }
    var mat = makeRT(FABRIC.RT.Mat44, float_array);
    mat.setTranslation(mat.getTranslation().multiplyScalar(options.scaleFactor));
    return mat;
  }
  
  var parseSkin = function(node) {
    var skin = {
      source: node.getAttribute('source'),
      sources: {},
      joints: []
    };
    var child = node.firstElementChild;
    
    while(child){
      switch (child.nodeName) {
        case 'bind_shape_matrix':
          skin.bind_shape_matrix = parseMatrix(child);
          break;
        case 'source':
          skin.sources[child.getAttribute('id')] = parseSource(child);
          break;
        case 'vertex_weights':
          skin.vertex_weights  = parseVertexWeights(child);
          break;
        case 'joints':
          skin.joints = parseJoints(child);
          break;
        default:
          warn("Warning in parseSkin: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return skin;
  }
  
  var parseController = function(node) {
    var controller = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'skin':
          controller.skin = parseSkin(child);
          break;
        default:
          warn("Warning in parseController: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return controller;
  }
  
  var parseLibaryControllers = function(node) {
    var libraryControllers = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'controller':
          libraryControllers[child.getAttribute('id')] = parseController(child);
          break;
        default:
          warn("Warning in parseLibaryControllers: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return libraryControllers;
  }
  
  var parseInstanceGeometry = function(node) {
    var instanceController = {
      url: node.getAttribute('url')
      };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        default:
          warn("Warning in parseInstanceGeometry: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return instanceController;
  }
  
  var parseInstanceController = function(node) {
    var instanceController = {
      url: node.getAttribute('url')
      };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        default:
          warn("Warning in parseInstanceController: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return instanceController;
  }
  
  var parseNode = function(node, nodeLibrary, parentId) {
    var nodeData = {
      name:  node.getAttribute('name'),
      type:  node.getAttribute('type'),
      instance_geometry: undefined,
      xfo: new FABRIC.RT.Xfo(),
      children:[]
    };
    if(parentId){
      nodeData.parentId = parentId;
    }
    var id = node.getAttribute('id');
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'translate': {
          var sid = child.getAttribute('sid');
          var str = child.textContent.split(new RegExp("\\s+"));
          var tr = new FABRIC.RT.Vec3(parseFloat(str[0]), parseFloat(str[1]), parseFloat(str[2]));
          tr = tr.multiplyScalar(options.scaleFactor);
          nodeData.xfo = nodeData.xfo.multiply(new FABRIC.RT.Xfo({tr:tr}));
          break;
        }
        case 'rotate': {
          var sid = child.getAttribute('sid');
          var str = child.textContent.split(new RegExp("\\s+"));
          var q = new FABRIC.RT.Quat().setFromAxisAndAngle(
                    new FABRIC.RT.Vec3(
                      parseFloat(str[0]),
                      parseFloat(str[1]),
                      parseFloat(str[2])),
                    Math.degToRad(parseFloat(str[3])));
          nodeData.xfo = nodeData.xfo.multiply(new FABRIC.RT.Xfo({ori:q}));
          break;
        }
        case 'scale': {
          var sid = child.getAttribute('sid');
          var str = child.textContent.split(new RegExp("\\s+"));
          var sc = new FABRIC.RT.Vec3(parseFloat(str[0]), parseFloat(str[1]), parseFloat(str[2]));
          nodeData.xfo = nodeData.xfo.multiply(new FABRIC.RT.Xfo({sc:sc}));
          break;
        }
        case 'matrix':
          nodeData.xfo.setFromMat44(parseMatrix(child));
          break;
        case 'instance_geometry':
          nodeData.instance_geometry = parseInstanceGeometry(child);
          break;
        case 'instance_controller':
          nodeData.instance_controller = parseInstanceController(child);
          break;
        case 'node':
          nodeData.children.push(parseNode(child, nodeLibrary, id));
          break;
        default:
          warn("Warning in parseNode: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    nodeLibrary[node.getAttribute('id')] = nodeData;
    return nodeData;
  }
  
  var parseVisualScene = function(node) {
    var scene = {
      nodes: [],  /* Hierarchial scene representation */
      nodeLibrary: {} /* flat scene representation for quick node lookup */
    };
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'node':
          scene.nodes.push(parseNode(child, scene.nodeLibrary));
          break;
        default:
          warn("Warning in parseVisualScene: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return scene;
  }
  
  var parseLibraryVisualScenes = function(node) {
    var scenes = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'visual_scene':
          scenes[child.getAttribute('id')] = parseVisualScene(child);
          break;
        default:
          warn("Warning in parseLibraryVisualScenes: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return scenes;
  }
  
  var parseScene = function(node) {
    var scene = {};
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'instance_visual_scene':
          scene.url = child.getAttribute('url');
          break;
        default:
          warn("Warning in parseScene: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return scene;
  }
  
  var parseColladaBase = function(node) {
    var colladaData = {}
    var child = node.firstElementChild;
    while(child){
      switch (child.nodeName) {
        case 'asset': 
          break;
        case 'library_effects':
          colladaData.libraryEffects = parseLibaryEffects(child);
          break;
        case 'library_materials': 
          colladaData.libraryMaterials = parseLibaryMaterials(child);
          break;
        case 'library_animations': 
          colladaData.libraryAnimations = parseLibaryAnimations(child);
          break;
        case 'library_geometries': 
          colladaData.libraryGeometries = parseLibaryGeometries(child);
          break;
        case 'library_controllers': 
          colladaData.libraryControllers = parseLibaryControllers(child);
          break;
        case 'library_visual_scenes':
          colladaData.libraryVisualScenes = parseLibraryVisualScenes(child);
          break;
        case 'visual_scenes':
          colladaData.visualScenes = parseVisualScenes(child);
          break;
        case 'scene':
          colladaData.scene = parseScene(child);
          break;
        default:
          warn("Warning in parseColladaBase: Unhandled node '" +child.nodeName + "'");
      }
      child = child.nextElementSibling;
    }
    return colladaData;
  }
  

  
  //////////////////////////////////////////////////////////////////////////////
  // SceneGraph Construction
  // This method returns an array of values from the given source data. 
  var getSourceData = function(source, id){
    var accessor = source.technique.accessor;
    var elemid = id * accessor.stride;
    return source.data.slice( elemid, elemid + accessor.stride );
  }
    
  var processGeometryData = function(meshData, trianglesData){
    var numTriangles = trianglesData.count;
    var attrcount = 0;
    var numUVsets = 0;
    var meshTriangleSourceData = {};
    var processedData = {
      constructionOptions: {},
      geometryData: {
        indices: []
      },
      vertexRemapping: []
    };
    var constructUVs = function(args){
      if(options.flipUVs){
        return new FABRIC.RT.Vec2(args[0],1.0-args[1]);
      }else{
        return new FABRIC.RT.Vec2(args[0],args[1]);
      }
    }
    var constructVec3 = function(args){
      return new FABRIC.RT.Vec3(args[0],args[1],args[2]);
    }
    var constructScaledVec3 = function(args){
      return new FABRIC.RT.Vec3(args[0]*options.scaleFactor,args[1]*options.scaleFactor,args[2]*options.scaleFactor);
    }
    var constructColor = function(args){
      return new FABRIC.RT.Color(args[0],args[1],args[2],args[3]);
    }
    for(var semantic in trianglesData.inputs){
      var input = trianglesData.inputs[semantic];
      var sourceName = input.source.slice(1);
      switch(semantic){
        case 'VERTEX':
          meshTriangleSourceData.positions = {
            source: meshData.sources[meshData.vertices.source.slice(1)],
            constructorFn: constructScaledVec3
          };
          processedData.geometryData.positions = [];
          break;
        case 'NORMAL':
          meshTriangleSourceData.normals = {
            source: meshData.sources[sourceName],
            constructorFn: constructVec3
          };
          processedData.geometryData.normals = [];
          break;
        case 'TEXCOORD':
          var uvset = 'uvs' + numUVsets;
          meshTriangleSourceData[uvset] = {
            source: meshData.sources[sourceName],
            constructorFn: constructUVs
          };
          processedData.geometryData[uvset] = [];
          processedData.constructionOptions.tangentsFromUV = numUVsets;
          numUVsets++;
          processedData.constructionOptions.uvSets = numUVsets;
          break;
        case 'COLOR':
          meshTriangleSourceData.vertexColors = {
            source: meshData.sources[sourceName],
            constructorFn: constructColor
          };
          processedData.geometryData.vertexColors = [];
          break;
        default:
          throw "Error: unhandled semantic '" + semantic +"'";
      }
      attrcount++
    }
    
    var indicesMapping = {};
    var vcount = 0;
    var vid = 0;
    
    for(var tid=0; tid<numTriangles; tid++){
      for(var j=0; j<3; j++){
        var attributeDataIndices = trianglesData.indices.slice( vid*attrcount, (vid*attrcount) + attrcount );
        var vertexMappingID = 'vid' + attributeDataIndices.join('_');
        // By using the attribute indices to generate a unique id for each vertex,
        // we can detect if a vertex is being reused. In the collada specification
        // all attributes have different counts, but in Fabric, all attributes have the
        // same count. To share a vertex, we must share all attribute data.
        if(!indicesMapping[vertexMappingID]){
          var vattrid = 0; // vertex attribute id
          for(var inputid in meshTriangleSourceData){
            var elementid = attributeDataIndices[vattrid];
            var sourceData = getSourceData(meshTriangleSourceData[inputid].source, elementid);
            var constructorFn = meshTriangleSourceData[inputid].constructorFn;
            processedData.geometryData[inputid].push(constructorFn(sourceData));
            vattrid++;
          }
          processedData.geometryData.indices.push(vcount);
          indicesMapping[vertexMappingID] = vcount;
          // We are remapping the vertices, and need to keep track of this so we can remap other
          // vertex data such as bone weights and indices
          processedData.vertexRemapping[vcount] = attributeDataIndices[0];
          vcount++;
        }
        else{
          processedData.geometryData.indices.push(indicesMapping[vertexMappingID]);
        }
        vid++;
      }
    }
    return processedData;
  }
  
  var constructGeometries = function(geometryData){
    var geometryNodes = [];
    if(geometryData.mesh){
      var meshData = geometryData.mesh;
      var constructGeometryNode = function(polygons){
        var name = geometryData.name;
        if(polygons.material != null){
          name += polygons.material;
        }
        var processedData = processGeometryData(meshData, polygons);
        processedData.constructionOptions.name = name;
        var geometryNode = scene.constructNode('Triangles', processedData.constructionOptions);
        if(processedData.geometryData.vertexColors){
          geometryNode.addVertexAttributeValue('vertexColors', 'Color', {
            genVBO:true
          });
        }
        geometryNode.loadGeometryData(processedData.geometryData);
        assetNodes[name] = geometryNode;
        geometryNodes.push(geometryNode);
      }
      if(meshData.triangles){
        for(var i=0; i<meshData.triangles.length; i++){
          constructGeometryNode(meshData.triangles[i]);
        }
      }
      if(meshData.polygons){
        for(var i=0; i<meshData.polygons.length; i++){
          constructGeometryNode(meshData.polygons[i]);
        }
      }
    }
    else{
      alert("This collada importer only supports polygon and triangle meshes.");
      throw "This collada importer only supports polygon and triangle meshes."
    }
    return geometryNodes;
  }

  
  var loadRigAnimation = function(rigNode){
    if(colladaData.libraryAnimations){
      if(!animationLibrary){
        animationLibrary = scene.constructNode('LinearKeyAnimationLibrary');
        assetNodes[animationLibrary.getName()] = animationLibrary;
      }
      if(!controllerNode){
        controllerNode = scene.constructNode('AnimationController');
        assetNodes[controllerNode.getName()] = controllerNode;
      }
      
      var skeletonNode = rigNode.getSkeletonNode();
      var bones = skeletonNode.getBones();
      var fksolver = rigNode.getSolver('solveColladaPose');
      if(!fksolver){
        fksolver = rigNode.addSolver('solveColladaPose', 'FKHierarchySolver');
      }
      // Construct the track set for this rig.
      var trackSet = new FABRIC.RT.KeyframeTrackSet(skeletonNode.getName()+'Animation');
      var xfoVarBindings = fksolver.getXfoVarBindings();
      var trackBindings = new FABRIC.RT.KeyframeTrackBindings();
      
      for (var i = 0; i < bones.length; i++) {
        var boneName = bones[i].name;
        var channels = colladaData.libraryAnimations.channelMap[boneName];
        
        if (!channels)
          continue;
        
        var trackIds = [];
        for (var channelName in channels) {
          var animation = channels[channelName];
          var sampler = animation.sampler;
          
          if (!sampler.INPUT || !sampler.OUTPUT || !sampler.INTERPOLATION)
            throw "Animation Channel must provide 'INPUT', 'OUTPUT', and 'INTERPOLATION' sources";
          
          // allright - now we need to create the data
          // let's check the first element of the INTERPOLATION semantic
          var inputSource = animation.sources[sampler.INPUT.source.slice(1)];
          var outputSource = animation.sources[sampler.OUTPUT.source.slice(1)];
          var interpolationSource = animation.sources[sampler.INTERPOLATION.source.slice(1)];
          
          var generateKeyframeTrack = function(channelName, keytimes, keyvalues, scaleFactor){
            var color;
            switch (channelName.substr(channelName.lastIndexOf('.')+1)) {
            case 'x': color = FABRIC.RT.rgb(1, 0, 0);        break;
            case 'y': color = FABRIC.RT.rgb(0, 1, 0);        break;
            case 'z': color = FABRIC.RT.rgb(0, 0, 1);        break;
            case 'w': color = FABRIC.RT.rgb(0.7, 0.7, 0.7);  break;
            default:
              throw 'unsupported channel:' + channelName;
            }
            
            // now let's reformat the linear data
            var track = new FABRIC.RT.KeyframeTrack(bones[i].name+'.'+channelName, color);
            var key = function(t, v){ return new FABRIC.RT.LinearKeyframe(t, v); }
            for (var j = 0; j < keytimes.length; j++) {
              track.keys.push(key(keytimes[j], keyvalues[j] * scaleFactor));
            }
            trackSet.tracks.push(track);
          //  trackIds[channelName] = trackSet.tracks.length - 1;
            trackIds.push(trackSet.tracks.length - 1);
          }
          
          
          // Check the channel type. TODO
          var channelType = channelName.substr(channelName.lastIndexOf('.') + 1).toUpperCase();
          switch(channelType){
          case 'ANGLE':
            for (var j = 0; j < outputSource.data.length; j++){
              outputSource.data[j] = Math.degToRad( outputSource.data[j] );
            }
            break;
          case 'MATRIX':
            // convert the input source to tr and ori keyframe tracks.
            var tr_x_keyvalues = [];
            var tr_y_keyvalues = [];
            var tr_z_keyvalues = [];
            var ori_x_keyvalues = [];
            var ori_y_keyvalues = [];
            var ori_z_keyvalues = [];
            var ori_w_keyvalues = [];
            var prevQuat;
            for (var j = 0; j < outputSource.technique.accessor.count; j++) {
              var matrixValues = getSourceData(outputSource, j);
              var mat = makeRT(FABRIC.RT.Mat44, matrixValues);
              var xfo = new FABRIC.RT.Xfo();
              xfo.setFromMat44(mat);
              tr_x_keyvalues.push(xfo.tr.x);
              tr_y_keyvalues.push(xfo.tr.y);
              tr_z_keyvalues.push(xfo.tr.z);
              
              if(j > 0){
                xfo.ori.alignWith(prevQuat);
              }
              prevQuat = xfo.ori;
              ori_x_keyvalues.push(xfo.ori.v.x);
              ori_y_keyvalues.push(xfo.ori.v.y);
              ori_z_keyvalues.push(xfo.ori.v.z);
              ori_w_keyvalues.push(xfo.ori.w);
            }
            generateKeyframeTrack('tr.x', inputSource.data, tr_x_keyvalues, options.scaleFactor);
            generateKeyframeTrack('tr.y', inputSource.data, tr_y_keyvalues, options.scaleFactor);
            generateKeyframeTrack('tr.z', inputSource.data, tr_z_keyvalues, options.scaleFactor);
            
            generateKeyframeTrack('ori.v.x', inputSource.data, ori_x_keyvalues, 1.0);
            generateKeyframeTrack('ori.v.y', inputSource.data, ori_y_keyvalues, 1.0);
            generateKeyframeTrack('ori.v.z', inputSource.data, ori_z_keyvalues, 1.0);
            generateKeyframeTrack('ori.w', inputSource.data, ori_w_keyvalues, 1.0);
            
            continue;
          }
        
          var scaleFactor = 1.0;
          // remap the target names
          switch(channelName){
          case 'rotation_x.ANGLE':
          case 'rotateX.ANGLE':
            channelName = 'ori.x';
            break;
          case 'rotation_y.ANGLE':
          case 'rotateY.ANGLE':
            channelName = 'ori.y';
            break;
          case 'rotation_z.ANGLE':
          case 'rotateZ.ANGLE':
            channelName = 'ori.z';
            break;
          case 'translation.X':
          case 'translate.X':
            channelName = 'tr.x';
            scaleFactor = options.scaleFactor;
            break;
          case 'translation.Y':
          case 'translate.Y':
            channelName = 'tr.y';
            scaleFactor = options.scaleFactor;
            break;
          case 'translation.Z':
          case 'translate.Z':
            channelName = 'tr.z';
            scaleFactor = options.scaleFactor;
            break;
          case 'scale.X':
            channelName = 'sc.x';
            break;
          case 'scale.Y':
            channelName = 'sc.y';
            break;
          case 'scale.Z':
            channelName = 'sc.z';
            break;
          }
          
          generateKeyframeTrack(channelName, inputSource.data, outputSource.data, scaleFactor);
        }
        trackBindings.addXfoBinding(xfoVarBindings[boneName], trackIds);
      }
      
      var trackSetID = animationLibrary.addTrackSet(trackSet, trackBindings);
      var variablesNode = rigNode.getVariablesNode();
      if(!variablesNode){
        variablesNode = rigNode.constructVariablesNode(rigNode.getName() + 'Variables', true);
        assetNodes[variablesNode.getName()] = variablesNode;
      }
      variablesNode.bindToAnimationTracks(animationLibrary, controllerNode, trackSetID);
      
    }
  }
  
  var controllerNode;
  var libraryRigs = {};
  var constructRigFromHierarchy = function(sceneData, rootNodeName, controllerName){
    if(!controllerName){
      controllerName = rootNodeName;
    }
    
    if(libraryRigs[rootNodeName]){
      return libraryRigs[rootNodeName];
    }
    
    // recurse on the hierarchy
    var boneIndicesMap = {};
    var globalXfos = [];
    var bones = [];
    var traverseChildren = function(nodeData, parentName) {
      var boneOptions = { name: nodeData.name, parent: -1, length: 0 };
      boneIndicesMap[nodeData.name] = bones.length;
      if (parentName) {
        boneOptions.parent = boneIndicesMap[parentName];
      }
      boneOptions.referenceLocalPose = nodeData.xfo;
      if (boneOptions.parent !== -1) {
        boneOptions.referencePose = bones[boneOptions.parent].referencePose.multiply(nodeData.xfo);
        
        // set the length of the parent bone based on the child bone local offset.
        if(Math.abs(nodeData.xfo.tr.x) > (Math.abs(nodeData.xfo.tr.y) + Math.abs(nodeData.xfo.tr.z)) &&
           Math.abs(nodeData.xfo.tr.x) > Math.abs(bones[boneOptions.parent].length)) {
          bones[boneOptions.parent].length = nodeData.xfo.tr.x;// * bones[boneOptions.parent].referencePose.sc.x;
        }
      }
      else {
        boneOptions.referencePose = nodeData.xfo;
      }
      bones.push(boneOptions);
      if (nodeData.children) {
        for (var i = 0; i < nodeData.children.length; i++) {
          traverseChildren(nodeData.children[i], nodeData.name);
        }
      }
    };
    traverseChildren(sceneData.nodeLibrary[rootNodeName]);
    // If any bones didn't get a size, then give them the length of the parent bone * 0.5
    for (i = 0; i < bones.length; i++) {
      if (bones[i].length === 0 && bones[i].parent != -1) {
        bones[i].length = bones[bones[i].parent].length * 0.75;
        
        // If the tip of the bone is below the floor, then 
        // shorten the bone till it touches the floor.
        var downVec = new FABRIC.RT.Vec3(0, -1, 0);
        var boneVec = bones[i].referencePose.ori.rotateVector(new FABRIC.RT.Vec3(bones[i].length, 0, 0));
        if(bones[i].referencePose.tr.y > 0 && boneVec.dot(downVec) > bones[i].referencePose.tr.y){
          bones[i].length *= bones[i].referencePose.tr.y / boneVec.dot(downVec);
        }
      }
      bones[i].radius = bones[i].length * 0.1;
    }
    
    var skeletonNode = scene.constructNode('CharacterSkeleton', {
      name:controllerName+"Skeleton",
      calcReferenceLocalPose: false,
      calcReferenceGlobalPose: false,
      calcInvMatrices: false
    });
    skeletonNode.setBones(bones);
    
    ///////////////////////////////
    
    var rigNode = scene.constructNode('CharacterRig', {
      name: controllerName+'CharacterRig',
      skeletonNode: skeletonNode
    });
    
    loadRigAnimation(rigNode);
    
    // Store the created scene graph nodes in the returned asset map.
    assetNodes[skeletonNode.getName()] = skeletonNode;
    assetNodes[rigNode.getName()] = rigNode;
    
    var rigData = {
      skeletonNode: skeletonNode,
      rigNode: rigNode
    };
    
    libraryRigs[rootNodeName] = rigData;
    return rigData;
  }


  var loadPoseOntoRig = function(sceneData, rigNode, rootNodeName){
    // recurse on the hierarchy
    var pose = [];
    var traverseChildren = function(nodeData, parentXfo) {
      var xfo = nodeData.xfo;
      if (parentXfo) {
        xfo = parentXfo.multiply(xfo);
      }
      pose.push(xfo);
      if (nodeData.children) {
        for (var i = 0; i < nodeData.children.length; i++) {
          traverseChildren(nodeData.children[i], xfo);
        }
      }
    };
    traverseChildren(sceneData.nodeLibrary[rootNodeName]);
    
    rigNode.setPose(pose);
  }


    
  var libraryControllers = {};
  var constructController = function(sceneData, url, name){
    
    // Lazy construction of geometries
    if(libraryControllers[url]){
      return libraryControllers[url];
    }
    
    var controllerData = colladaData.libraryControllers[url].skin;
    
    
    ////////////////////////////////////////////////////////////////////////////
    // Construct the Rig
    var jointDataSource = controllerData.sources[controllerData.vertex_weights.inputs.JOINT.source.slice(1)];
    
    // Look up through the hierarchy and find the actual root.
    var joint =  sceneData.nodeLibrary[jointDataSource.data[0]];
    if(!joint){
      throw "Joint nodes not exported with collada file.";
    }
    while(joint.parentId && joint.parentId != "Scene_Root"){
      joint = sceneData.nodeLibrary[joint.parentId];
    }
    var skeletonData = constructRigFromHierarchy(sceneData, joint.name, name);
    
    
    var bindPoseDataSource = controllerData.sources[controllerData.joints.INV_BIND_MATRIX.source.slice(1)];
    var invmatrices = [];
    for (var j = 0; j < jointDataSource.data.length; j++) {
      var bindPoseValues = getSourceData(bindPoseDataSource, j);
      var mat = makeRT(FABRIC.RT.Mat44, bindPoseValues);//.transpose();
      mat.setTranslation(mat.getTranslation().multiplyScalar(options.scaleFactor));
      invmatrices[j] = mat.multiply(controllerData.bind_shape_matrix);
    }
    
  
    ////////////////////////////////////////////////////////////////////////////
    // Set up the vertex weights.
    // In fabric we store the bind skinning wieghts and indices in 2 vec4 values
    // per vertex, and this is primarily so we can use the GPU for enveloping.
    // Here must convert the source data, which can have any number of bone influences
    // per vertex, down to 4. 
    var vertexWeightsSource = controllerData.sources[controllerData.vertex_weights.inputs.WEIGHT.source.slice(1)];
    var bones = skeletonData.skeletonNode.getBones();
    var boneIds = [];
    var boneWeights = [];
    
    var  bubbleSortWeights = function(weights, indices, start, end) {
      if (start != end - 1) {
        for (var i = start; i < end; i++) {
          for (var j = i + 1; j < end; j++) {
            if (weights[i] < weights[j]) {
              // swap them!
              var tmpScalar = weights[i];
              var tmpIndex = indices[i];
              weights[i] = weights[j];
              indices[i] = indices[j];
              weights[j] = tmpScalar;
              indices[j] = tmpIndex;
            }
          }
        }
      }
    }

    // the vcount table tells us how many bindings deform this vertex.
    // the vertices array is a pair of indices for each joint binding.
    // The first is the JOINT, and then 2nd the WEIGHT
    var makeVec4 = function(data) {
      // The following is a bit of a hack. Not sure if we can combine new and apply.
      if (data.length === 0) return new FABRIC.RT.Vec4();
      if (data.length === 1) return new FABRIC.RT.Vec4(data[0], 0, 0, 0);
      if (data.length === 2) return new FABRIC.RT.Vec4(data[0], data[1], 0, 0);
      if (data.length === 3) return new FABRIC.RT.Vec4(data[0], data[1], data[2], 0);
      return new FABRIC.RT.Vec4(data[0], data[1], data[2], data[3]);
    };
    
    var bid = 0;
    for (var i = 0; i < controllerData.vertex_weights.vcounts.length; i++) {
      var boneIdsArray = [];
      var boneWeightsArray = [];
      var numbindings = controllerData.vertex_weights.vcounts[i];
      for (var j = 0; j < numbindings; j++) {
        var jointid = controllerData.vertex_weights.indices[bid];
        var jointweightid = controllerData.vertex_weights.indices[bid+1];
        if(jointid > jointDataSource.data.length){
          throw "ERRROR";
        }
        boneIdsArray[j] = jointid;
        boneWeightsArray[j] = vertexWeightsSource.data[jointweightid];
        bid += 2;
      }
      bubbleSortWeights(boneWeightsArray, boneIdsArray, 0, numbindings);
      boneIds.push(makeVec4(boneIdsArray));
      boneWeights.push(makeVec4(boneWeightsArray));
    }
    
    /////////////////////////////////////
    // set inverse binding matrices
    var jointRemapping = [];
    for (var i = 0; i < jointDataSource.data.length; i++) {
      for (var j = 0; j < bones.length; j++) {
        if(bones[j].name==jointDataSource.data[i]){
          jointRemapping[i] = j;
          break;
        }
      }
      if(j==bones.length){
        warn("Joints '"+jointDataSource.data[i]+"' not found in skeleton");
        jointRemapping[i] = -1;
      }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Get the geometry data that this skin mesh is based on
    var geometryData = colladaData.libraryGeometries[controllerData.source.slice(1)];
    
    var sourceMeshArray = geometryData.mesh.triangles ? geometryData.mesh.triangles : geometryData.mesh.polygons;
    if(!sourceMeshArray){
      throw "No gometry specified";
    }
      
    
    var constructSkinnedGeometry = function(polygons){
      var name = geometryData.name;
      if(polygons.material != null){
        name += polygons.material;
      }
      var processedData = processGeometryData(geometryData.mesh, polygons);
      
      // Now remap the generated arrays to the vertices in the mesh we store.
      processedData.geometryData.boneIds = [];
      processedData.geometryData.boneWeights = [];
      var vid = 0;
      for (var i = 0; i < processedData.vertexRemapping.length; i++) {
        var vid = processedData.vertexRemapping[i];
        processedData.geometryData.boneIds.push(boneIds[vid]);
        processedData.geometryData.boneWeights.push(boneWeights[vid]);
      }
      
      processedData.constructionOptions.name = name;
      var characterMeshNode = scene.constructNode('CharacterMesh', processedData.constructionOptions);
      if(processedData.geometryData.vertexColors){
        characterMeshNode.addVertexAttributeValue('vertexColors', 'Color', {
          genVBO:true
        });
      }
      characterMeshNode.loadGeometryData(processedData.geometryData);
      
      characterMeshNode.setInvMatrices(invmatrices, jointRemapping);
      assetNodes[name] = characterMeshNode;
      return characterMeshNode;
    }
    
    
    var geometries = [];
    for(var i=0; i<sourceMeshArray.length; i++){
      geometries.push(constructSkinnedGeometry(sourceMeshArray[i]));
    }
    
    var controllerNodes = {
      geometries: geometries,
      skeletonData: skeletonData,
      controllerData: controllerData
    }
    
    libraryControllers[url] = controllerNodes;
    return controllerNodes;
  }
  
  // Construct the Geometries.
  var libraryGeometries = {};
  var getGeometryNodes = function(geomid){
    // Lazy construction of geometries
    if(!libraryGeometries[geomid]){
      libraryGeometries[geomid] = constructGeometries(colladaData.libraryGeometries[geomid]);
    }
    return libraryGeometries[geomid];
  }
  
  
  // Construct the Scene
  var constructScene = function(sceneData){
    
    var constructInstance = function(instanceData, parentTransformNode){
      if(instanceData.instance_geometry){
        var url = instanceData.instance_geometry.url.slice(1);
        var geometries = getGeometryNodes(url);
        for(var i=0; i<geometries.length; i++){
          var geometryNode = geometries[i];
          var materialNode;
          if(instanceData.instance_geometry.instance_material){
            // TODO:
          }
          
          var transformNodeOptions = { name: instanceData.name +"Transform" };
          if(parentTransformNode){
            transformNodeOptions.hierarchical = true;
            transformNodeOptions.localXfo = instanceData.xfo;
            transformNodeOptions.parentTransformNode = parentTransformNode;
          }else{
            if(options.parentTransformNode){
              transformNodeOptions.hierarchical = true;
              transformNodeOptions.parentTransformNode = options.parentTransformNode;
            }else{
              transformNodeOptions.hierarchical = false;
              transformNodeOptions.globalXfo = instanceData.xfo;
            }
          }
          var transformNode = scene.constructNode('Transform', transformNodeOptions );
          if(geometryNode/* && materialNode*/){
            var instanceNode = scene.constructNode('Instance', {
              name: instanceData.name, 
              transformNode: transformNode,
              geometryNode: geometryNode,
              materialNode: materialNode
            });
            assetNodes[instanceData.name] = instanceNode;
          }
        }
      
      }
      else if(instanceData.instance_controller){
        var url = instanceData.instance_controller.url.slice(1);
        var controllerNodes = constructController(sceneData, url, instanceData.name);
        
        for(var i=0; i<controllerNodes.geometries.length; i++){
          geometryNode = controllerNodes.geometries[i];
          if(instanceData.instance_controller.instance_material){
            // TODO: materialNode =
          }
          var characterNode = scene.constructNode('CharacterInstance', {
              name: geometryNode.getName()+'CharacterInstance',
              geometryNode: geometryNode,
              materialNode: materialNode,
              rigNode: controllerNodes.skeletonData.rigNode
            });
          assetNodes[characterNode.getName()] = characterNode;
        }
      }
      
      for(var i=0; i<instanceData.children.length; i++){
        if(instanceData.children[i].type != 'JOINT'){
          constructInstance(instanceData.children[i], transformNode);
        }
      }
    }
    for(var i=0; i<sceneData.nodes.length; i++){
      if(sceneData.nodes[i].type != 'JOINT'){
        constructInstance(sceneData.nodes[i]);
      }
    }
  }
  
  var xmlText, colladaData;
  var parseAndConstruct = function(){
    var parser = new DOMParser();
    var xmlDoc = parser.parseFromString(xmlText, 'text/xml');
    
    // get the root and check its type
    var xmlRoot = xmlDoc.firstChild;
    if (xmlRoot.nodeName != 'COLLADA') {
      throw 'Collada file is corrupted.';
    }
    colladaData = parseColladaBase(xmlRoot);
  
    if(colladaData.scene){
      var sceneData = colladaData.libraryVisualScenes[colladaData.scene.url.slice(1)];
      if(options.constructScene){
        constructScene(sceneData);
        
        // The file may contain a hierarchy that can be used to generate a skeleton
        if (options.constructRigFromHierarchy) {
          constructRigFromHierarchy(sceneData, options.rigHierarchyRootNodeName);
        }
      }
      else{
        
        if (options.loadAnimationUsingRig) {
          loadRigAnimation(options.rigNode);
        }
        if (options.loadPoseOntoRig) {
          loadPoseOntoRig(sceneData, options.rigNode, options.rigHierarchyRootNodeName);
        }
      }
    }
  }
  
  if(callback){
    FABRIC.loadResourceURL(assetFile, 'text/xml', function(txt){
      xmlText = txt;
      parseAndConstruct();
      callback(assetNodes);
    });
    
  }else{
    xmlText = FABRIC.loadResourceURL(assetFile, 'text/xml');
    parseAndConstruct();
    return assetNodes;
  }
  
});

