/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Geometry"], function() {

FABRIC.SceneGraph.registerNodeType('ObjResource', {
  briefDesc: 'The ObjResource node is a resource load node able to load OBJ files.',
  detailedDesc: 'The ObjResource node is a resource load node able to load OBJ files. It utilizes a C++ based extension to load OBJ files in a very fast fashion.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
    removeParsersOnLoad: 'If set to true, the parser operator will be removed after parsing.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      splitObjects: false,
      splitMaterials: false,
      splitGroups: false,
      printDetailedInfo: false
    });

    var resourceLoadNode = scene.constructNode('ResourceLoad', options ),
    resourceloaddgnode = resourceLoadNode.getDGLoadNode();
    
    resourceloaddgnode.addMember('reload', 'Boolean', true);
    resourceloaddgnode.addMember('handle', 'OBJDataHandle');
  
    resourceloaddgnode.addMember('splitObjects', 'Boolean', options.splitObjects);
    resourceloaddgnode.addMember('splitGroups', 'Boolean', options.splitMaterials);
    resourceloaddgnode.addMember('splitMaterials', 'Boolean', options.splitGroups);
    resourceloaddgnode.addMember('printDetailedInfo', 'Boolean', options.printDetailedInfo);
    
    resourceloaddgnode.addMember('objectNames', 'String[]');
    resourceloaddgnode.addMember('groupNames', 'String[]');
    resourceloaddgnode.addMember('materialNames', 'String[]');
    
    resourceloaddgnode.bindings.append(scene.constructOperator({
        operatorName: 'parseObj',
        srcFile: 'FABRIC_ROOT/SG/KL/loadObj.kl',
        entryPoint: 'parseObj',
        parameterLayout: [
          'self.resource',
          'self.handle',
          'self.splitObjects',
          'self.splitGroups',
          'self.splitMaterials',
          'self.reload',
          'self.printDetailedInfo',
          
          'self.objectNames',
          'self.groupNames',
          'self.materialNames'
        ],
        async: false
      }));
    
    // Note: this is a hack to work arround an issue in Chrome.
    // we cannot make synchronous requests while asynch requests are in progress.
    // so we pre-construct the operators here
    scene.constructOperator({
        operatorName: 'setObjGeom',
        srcFile: 'FABRIC_ROOT/SG/KL/loadObj.kl',
        entryPoint: 'setObjGeom',
        parameterLayout: [
          'self',
          'resource.handle',
          'uniforms.entityIndex',
          'uniforms.indices',
          'self.positions<>',
          'self.normals<>',
          'self.uvs0<>',
          'resource.reload'
        ],
        async: false
      });
    
    var refCnt = 0;
    resourceLoadNode.incrementRef = function(){
      refCnt++;
    }
    resourceLoadNode.decrementRef = function(){
      refCnt--;
      if(refCnt===0){
        resourceLoadNode.pub.addEventListener('loadSuccess', function(){
          resourceLoadNode.disposeData();
        });
      }
    }
    
    resourceLoadNode.disposeData = function(){
      // this frees up the memory used by the resource.
      resourceloaddgnode.removeMember('resource');
      resourceloaddgnode.removeMember('handle');
      resourceloaddgnode.bindings.remove(0);
    }
    
    var parentWriteData = resourceLoadNode.writeData;
    resourceLoadNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      constructionOptions.splitObjects = options.splitObjects;
      constructionOptions.splitMaterials = options.splitMaterials;
      constructionOptions.splitGroups = options.splitGroups;
    };
    
    var objParsed = false;
    var loadedGeometries = {}, objectNames, materialNames;

    resourceLoadNode.pub.addEventListener('loadSuccess', function(){
      try{
        resourceloaddgnode.evaluate();
      }catch(e){
        console.warn(e);
      }
      resourceLoadNode.pub.addEventListener('objloadsuccess', function(evt){
        if(evt.objectNames.length > 0){
          for(var i=0; i<evt.objectNames.length; i++){
            var objectName;
            if(evt.materialNames.length == evt.objectNames.length && evt.materialNames[i].length > 0){
              objectName = evt.materialNames[i];
            }else if(evt.groupNames.length == evt.objectNames.length && evt.groupNames[i].length > 0){
              objectName = evt.groupNames[i];
            }else if(evt.objectNames[i].length > 0){
              objectName = evt.objectNames[i];
            }else{
              objectName = options.baseName;
            }
            loadedGeometries[objectName] = scene.constructNode('ObjTriangles', {
              resourceLoadNode: resourceLoadNode.pub,
              entityIndex: i,
              name: objectName
            }).pub;
            evt.objectNames[i] = objectName;
          }
        }else{
          evt.objectNames = [options.baseName];
          loadedGeometries[options.baseName] = scene.constructNode('ObjTriangles', {
            resourceLoadNode: resourceLoadNode.pub,
            entityIndex: -1,
            name: options.baseName
          }).pub;
        }
        objParsed = true;
        objectNames = evt.objectNames;
        materialNames = evt.materialNames;
        resourceLoadNode.pub.fireEvent('objparsesuccess', {
          loadedGeometries: loadedGeometries,
          objectNames: objectNames,
          materialNames: materialNames
        })
        return 'remove';
      });

      resourceLoadNode.pub.fireEvent('objloadsuccess', {
        objectNames: resourceloaddgnode.getData('objectNames'),
        groupNames: resourceloaddgnode.getData('groupNames'),
        materialNames: resourceloaddgnode.getData('materialNames')
      });
      
      return "remove";
    });
    var addEventListener = resourceLoadNode.pub.addEventListener;
    resourceLoadNode.pub.addEventListener = function(type, fn) {
      if(type === 'objparsesuccess' && objParsed) {
        fn({
          loadedGeometries: loadedGeometries,
          objectNames: objectNames,
          materialNames: materialNames
        });
      }
      else {
        addEventListener(type, fn);
      }
    }
    return resourceLoadNode;
  }
});

FABRIC.SceneGraph.registerNodeType('ObjTriangles', {
  briefDesc: 'The ObjLoadTriangles node is a resource load node able to load OBJ files.',
  detailedDesc: 'The ObjLoadTriangles node is a resource load node able to load OBJ files. It utilizes a C++ based extension to load OBJ files in a very fast fashion.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
    removeParsersOnLoad: 'If set to true, the parser operator will be removed after parsing.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      resourceLoadNode: undefined,
      removeParsersOnLoad: false,
      entityIndex: -1
    });
    
    options.uvSets = 1; //To refine... what if there is no UV set??
    var trianglesNode = scene.constructNode('Triangles', options);

    trianglesNode.pub.addUniformValue('entityIndex', 'Integer', options.entityIndex);
    
    var resourceLoadNode;
    trianglesNode.pub.getResourceLoadNode = function() {
      return resourceLoadNode;
    };
    trianglesNode.pub.setResourceLoadNode = function(node) {
      if (!node || !node.isTypeOf('ResourceLoad')) {//In fact we should check typeOf('ObjResource') but since the node is not fully constructed its "typeOf" function might not have been updated...
        throw ('Must pass in a ObjResource resource loader');
      }

      resourceLoadNode = scene.getPrivateInterface(node);
      resourceLoadNode.incrementRef();
      trianglesNode.getAttributesDGNode().setDependency( resourceLoadNode.getDGLoadNode(), 'resource');
    
      var attributesdgnode = trianglesNode.getAttributesDGNode();
      attributesdgnode.bindings.append(scene.constructOperator({
        operatorName: 'setObjGeom',
        srcFile: 'FABRIC_ROOT/SG/KL/loadObj.kl',
        entryPoint: 'setObjGeom',
        parameterLayout: [
          'self',
          'resource.handle',
          'uniforms.entityIndex',
          'uniforms.indices',
          'self.positions<>',
          'self.normals<>',
          'self.uvs0<>',
          'resource.reload'
        ]
      }));
      if(options.removeParsersOnLoad){
        var numOperators = attributesdgnode.bindings.getLength();
        resourceLoadNode.pub.addEventListener('objparsesuccess', function(){
          attributesdgnode.evaluate();
          
          // TODO: Log an issue to enable removal of operators by object or name.
          attributesdgnode.bindings.remove(numOperators-1);
          attributesdgnode.bindings.remove(numOperators-2);
          
          
          trianglesNode.getAttributesDGNode().removeDependency('resource');
          resourceLoadNode.decrementRef();
        });
      }
    }
    
    
    //////////////////////////////////////////
    // Persistence
    var parentAddDependencies = trianglesNode.addDependencies;
    trianglesNode.addDependencies = function(sceneSerializer) {
      parentAddDependencies(sceneSerializer);
      sceneSerializer.addNode(resourceLoadNode.pub);
    };
    
    var parentWriteData = trianglesNode.writeData;
    var parentReadData = trianglesNode.readData;
    trianglesNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      if(sceneSerializer.isNodeBeingSaved(resourceLoadNode.pub)){
        constructionOptions.entityIndex = options.entityIndex;
        nodeData.resourceLoadNode = resourceLoadNode.pub.getName();
      }
    };
    trianglesNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      if(nodeData.resourceLoadNode){
        trianglesNode.pub.setResourceLoadNode(sceneDeserializer.getNode(nodeData.resourceLoadNode));
      }
    };
    
    /////////////////////////////////////////////////////////////////////

    if(options.resourceLoadNode){
      trianglesNode.pub.setResourceLoadNode(options.resourceLoadNode);
    }
    
    return trianglesNode;
  }
});


FABRIC.SceneGraph.registerParser('obj', function(scene, assetUrl, options, callback) {
  options.url = assetUrl;
  var resourceLoadNode = scene.constructNode('ObjResource', options );
  resourceLoadNode.addEventListener('objparsesuccess', function(evt){
    callback(evt.loadedGeometries);
  });
  return resourceLoadNode;
});

});
