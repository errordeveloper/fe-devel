
//
// Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
//

//////////////////////////////////////////////////////////////////////////////
// The SceneSerializer is a tool for writing out all the data for a collection
// of nodes that the developer wishes to save.
// Note: it is the responsibility of the developer to determine which nodes
// need to be persisted and which ones done't.
// For example, a scene graph might always construct a Viewport or Grid object
// which shouldn't be persisted.
//
// Example usage
//   var sceneSerializer = FABRIC.SceneGraph.constructManager('SceneSerializer');
//  for(i in savableNodes){
//    sceneSerializer.addNode(savableNodes[i]);
//  }
//  // Now put the data somewhere. Maybe in a database or file.
//  FABRIC.FS.write(sceneSerializer.save());

/**
 * Constructor for a SceneSerializer object.
 * @param {object} scene The scene to save.
 * @return {object} The created SceneSerializer object.
 */
FABRIC.SceneGraph.registerManagerType('SceneSerializer', {
  briefDesc: 'The SceneSerializer manages the persistence of objects.',
  detailedDesc: '.',
  parentManagerDesc: '',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      filteredNodeTypes: [],
      typeRemappings: {}
    });
  
    var filteredNodes = [];
    var savedNodes = [];
    var savedData = [];
    var nodeCountsByType = {};
    var storedDGNodes = {};
    var isNodeBeingSaved = function(node) {
      return (savedNodes.indexOf(node) !== -1);
    };
    var isNodeExcluded = function(node) {
      if(filteredNodes.indexOf(node) === -1){
        for(var i=0; i<options.filteredNodeTypes.length; i++){
          if (node.isTypeOf(options.filteredNodeTypes[i])) {
            return true;
          }
        }
        return false;
      }
      return true;
    };
    var sceneSerializer = {
      addNode: function(node) {
        if (!node || !node.isTypeOf || !node.isTypeOf('SceneGraphNode')) {
          throw 'SceneSaver can only save SceneGraphNodes';
        }
        if(!isNodeBeingSaved(node) && !isNodeExcluded(node)){
          var constructionOptions = {};
          var nodeData = {};
          var nodePrivate = scene.getPrivateInterface(node);
          nodePrivate.addDependencies(sceneSerializer);
          savedNodes.push(node);
          if(!nodeCountsByType[node.getType()]){
            nodeCountsByType[node.getType()] = 1;
          }
          else{
            nodeCountsByType[node.getType()]++;
          }
        };
      },
      isNodeBeingSaved: function(node) {
        return isNodeBeingSaved(node);
      },
      isNodeExcluded: function(node) {
        return isNodeExcluded(node);
      },
      writeDGNodesData: function(sgnodeName, desc) {
        if(!storedDGNodes[sgnodeName]){
          storedDGNodes[sgnodeName] = {};
          for(var dgnodeName in desc){
            storedDGNodes[sgnodeName][dgnodeName] = desc[dgnodeName];
          }
        }
      },
      getTypeRemapping: function(type){
        if(options.typeRemappings[type]){
          return options.typeRemappings[type];
        }
        return type;
      },
      pub:{
        // Add the filter nodes first, and then add the nodes you wish to save.
        filterNode: function(node) {
          return filteredNodes.push(node);
        },
        addNode: function(node) {
          return sceneSerializer.addNode(node);
        },
        wrapQuotes: function(val) {
          return '\"' + ((typeof val === 'string') ? val : val.toString()) + '\"';
        },
        serialize: function() {
          for(var i=0;i<savedNodes.length;i++) {
            var nodePrivate = scene.getPrivateInterface(savedNodes[i]);
            var constructionOptions = { };
            var nodeData = {};
            nodePrivate.writeData(sceneSerializer, constructionOptions, nodeData);
            savedData[i] = {
              options: constructionOptions,
              data: nodeData
            };
          }
        },
        save: function(writer) {
          this.serialize();
          var binaryStorageNode;
          if(writer.getBinaryStorageNode){
            binaryStorageNode = scene.getPrivateInterface(writer.getBinaryStorageNode());
          }
          var str = '{';
          str += '\n  \"metadata\":{';
          str += '\n    \"numNodes:\":' + savedNodes.length;
          str += ',\n    \"nodeCountsByType\":' + JSON.stringify(nodeCountsByType);
          if(binaryStorageNode){
            str += ',\n    \"binaryStorage\":' + true;
            str += ',\n    \"binaryFilePath\":' + this.wrapQuotes(writer.getBinaryFileName());
          }
          str += '\n  },';
          str += '\n  \"sceneGraphNodes\":[';
          for (var i = 0; i < savedNodes.length; i++) {
            if (i > 0) {
              str += ',';
            }
            var name = savedNodes[i].getName();
            var type = sceneSerializer.getTypeRemapping(savedNodes[i].getType());
            str += '\n    {';
            str += '\n      \"name\":' + this.wrapQuotes(name);
            str += ',\n      \"type\":' + this.wrapQuotes(type);
            str += ',\n      \"options\":' + JSON.stringify(savedData[i].options);
            str += ',\n      \"data\":' + JSON.stringify(savedData[i].data);
            
            if(storedDGNodes[name]){
              
              if(binaryStorageNode){
                binaryStorageNode.storeDGNodes( name, storedDGNodes[name]);
              }else{
                str += ',\n      \"dgnodedata\":{';
                var nodecnt = 0;
                for(var dgnodename in storedDGNodes[name]){
                  var dgnodeDataDesc = storedDGNodes[name][dgnodename];
                  var dgnode = dgnodeDataDesc.dgnode;
                  if (nodecnt > 0) {
                    str += ',';
                  }
                  str += '\n        \"'+dgnodename+'\":{';
                  str += '\n          \"sliceCount\":' + dgnode.getCount();
                  if(dgnodeDataDesc.members){
                    str += ',\n          \"memberData\":' + JSON.stringify(dgnode.getMembersBulkData(dgnodeDataDesc.members));
                  }
                  else{
                    str += ',\n          \"memberData\":' + JSON.stringify(dgnode.getBulkData());
                  }
                  str += '\n        }';
                  nodecnt++;
                }
                
                str += '\n      }';
            
              }
            }
            str += '\n    }';
            if(i%100==0){
              FABRIC.flush();
            }
          }
          str += '\n  ]';
          str += '\n}';
          writer.write(str);
          return true;
        },
        saveBinary: function(writer) {
          this.serialize();
          var binaryStorageNode;
          if(writer.getBinaryStorageNode){
            binaryStorageNode = scene.getPrivateInterface(writer.getBinaryStorageNode());
          }
          for (var i = 0; i < savedNodes.length; i++) {
            var name = savedNodes[i].getName();
            if(storedDGNodes[name]){
              if(binaryStorageNode){
                binaryStorageNode.storeDGNodes( name, storedDGNodes[name]);
              }
            }
            if(i%100==0){
              FABRIC.flush();
            }
          }
          writer.writeBinary();
          return true;
        }
      }
    };
    return sceneSerializer;
  }});


//////////////////////////////////////////////////////////////////////////////
// The SceneDeserializer is a tool for loading a collection of nodes and
// re-binding dependencies.
//
// Example usage
//   var sceneDeserializer = FABRIC.SceneGraph.constructManager('SceneDeserializer');
//  var loadedNodes = sceneDeserializer.load();

/**
 * Constructor for a SceneDeserializer object.
 * @param {object} scene The scene to load into.
 * @param {object} reader The reader object to read from.
 * @return {object} The created SceneDeserializer object.
 */
FABRIC.SceneGraph.registerManagerType('SceneDeserializer', {
  briefDesc: 'The SceneDeserializer manages the loading of persisted objects.',
  detailedDesc: '.',
  parentManagerDesc: '',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      preLoadScene: false
    });
    
    // Preloading nodes enables data to be loaded into existing nodes.
    // This is usefull when saving presets, rather then scenegraph descriptions.
    var preLoadedNodes = {};
    var dataObj;
  
    var constructedNodeMap = {};
    var nodeNameRemapping = {};
    var nodeDataMap = {};
    
    if(options.preLoadScene){
      var sceneNodes = scene.getSceneGraphNodes();
      for(var name in sceneNodes){
        preLoadedNodes[name] = sceneNodes[name].pub;
      }
    };
    var nodeData;
    var loadNodeBinaryFileNode;
    var sgnodeDataMap = {};
    var sceneDeserializer = {
      getNode: function(nodeName) {
        nodeName = nodeNameRemapping[ nodeName ]
        if (constructedNodeMap[nodeName]) {
          return constructedNodeMap[nodeName];
        }else {
          return scene.pub.getSceneGraphNode(nodeName);
        }
      },
      loadDGNodesData: function(sgnodeName, desc) {
        if(dataObj.metadata.binaryStorage){
          loadNodeBinaryFileNode.pub.addOnLoadSuccessCallback(function(){
            loadNodeBinaryFileNode.pub.loadDGNodes(sgnodeName, desc);
          });
        }
        else{
          var nodeData = sgnodeDataMap[sgnodeName];
          if(!nodeData.dgnodedata){
            console.warn("missing dgnode data for node:" + sgnodeName);
            return;
          }
          for(var dgnodename in desc){
            var data = nodeData.dgnodedata[dgnodename];
            var dgnode = desc[dgnodename].dgnode;
            dgnode.setCount(data.sliceCount);
            var members = dgnode.getMembers();
            var memberData = {};
            for(var memberName in members){
              if(data.memberData[memberName]){
                memberData[memberName] = data.memberData[memberName];
              }
            }
            dgnode.setBulkData(memberData);
          }
        }
      },
      pub: {
        setPreLoadedNode: function(node, nodeName) {
          if(!nodeName) nodeName = node.getName();
          preLoadedNodes[nodeName] = node;
        },
        load: function(storage, callback) {
          storage.read(function(data){
            if(!data){
              return;
            }
            dataObj = data;
            if(dataObj.metadata.binaryStorage){
              
              var binaryFilePath = dataObj.metadata.binaryFilePath;
              if(storage.getUrl){
                var pathArray = storage.getUrl().split('/');
                pathArray.pop();
                binaryFilePath = pathArray.join('/') + '/' + binaryFilePath;
              }
              loadNodeBinaryFileNode = scene.constructNode('LoadBinaryDataNode', {
                url: binaryFilePath,
                secureKey: 'secureKey'
              });
            }
            var remainingNodes = dataObj.sceneGraphNodes.length;
            var loadDGNode = function(nodeData){
              nodeDataMap[nodeData.name] = nodeData;
              FABRIC.createAsyncTask(function(){
                var node = preLoadedNodes[nodeData.name];
                if (!node) {
                  node = scene.pub.constructNode(nodeData.type, nodeData.options);
                }
                // in case a name collision occured, store a name remapping table.
                nodeNameRemapping[ nodeData.name ] = node.getName();
                var nodePrivate = scene.getPrivateInterface(node);
                nodePrivate.readData(sceneDeserializer, nodeData.data);
                constructedNodeMap[node.getName()] = node;
                remainingNodes--;
                if(remainingNodes == 0){
                  if(loadNodeBinaryFileNode){
                    loadNodeBinaryFileNode.pub.addOnLoadSuccessCallback(function(){
                      loadNodeBinaryFileNode.disposeData();
                    });
                  }
                  if(callback)
                    callback(constructedNodeMap);
                }
              });
            }
            for (var i = 0; i < dataObj.sceneGraphNodes.length; i++) {
              // Generate a map of the array of data that can be easily re-indexed
              sgnodeDataMap[dataObj.sceneGraphNodes[i].name] = dataObj.sceneGraphNodes[i];
              loadDGNode(dataObj.sceneGraphNodes[i]);
            }
          });
        }
      }
    };
    return sceneDeserializer;
  }});

/**
 * Constructor to create a logWriter object.
 * @constructor
 */
FABRIC.SceneGraph.LogWriter = function() {
  var str = "";
  this.write = function(instr) {
    str = instr;
    console.log(str);
  }
  this.log = function(instr) {
    console.log(str);
  }
};

FABRIC.SceneGraph.LocalStorage = function(name) {
  this.write = function(instr) {
    localStorage.setItem(name, instr);
  }
  this.read = function(callback){
    callback(JSON.parse(localStorage.getItem(name)));
  }
  this.log = function() {
    console.log(localStorage.getItem(name));
  }
};

/**
 * Constructor to create a FileWriter object.
 * @constructor
 * @param {string} filepath The path to the file to write to.
 */
FABRIC.SceneGraph.FileWriter = function(scene, title, suggestedFileName) {
  
  var path;
  var str = "";
//  this.querySavePath = function(instr) {
    path = scene.IO.queryUserFileAndFolderHandle(scene.IO.forOpenWithWriteAccess, title, "json", suggestedFileName);
//  }
  this.write = function(instr) {
    str = instr;
    scene.IO.putTextFile(str, path);
  }
  this.log = function() {
    console.log(str);
  }
};

FABRIC.SceneGraph.FileWriterWithBinary = function(scene, title, suggestedFileName, options) {
  
  var path = scene.IO.queryUserFileAndFolderHandle(scene.IO.forOpenWithWriteAccess, title, "json", suggestedFileName);
  var jsonFilename = path.fileName.split('.')[0];
  var binarydatapath = scene.IO.queryUserFileAndFolderHandle(scene.IO.forOpenWithWriteAccess, "Secure ", "fez", jsonFilename);
  
  var writeBinaryDataNode = scene.constructNode('WriteBinaryDataNode', options);
      
  var str = "";
  this.getBinaryStorageNode = function(){
    return writeBinaryDataNode;
  }
  
  this.getBinaryFileName = function(){
    return binarydatapath.fileName;
  }
  
  this.write = function(instr) {
    str = instr;
    scene.IO.putTextFile(str, path);
    FABRIC.flush();
    writeBinaryDataNode.write('resource', binarydatapath);
  }
  this.writeJSON = function(instr) {
    str = instr;
    scene.IO.putTextFile(str, path);
  }
  
  this.writeBinary = function() {
    writeBinaryDataNode.write('resource', binarydatapath);
  }
  
  this.log = function(instr) {
    console.log(str);
  }
};

/**
 * Constructor to create a FileReader object.
 * @constructor
 * @param {string} filepath The path to the file to read from.
 */
FABRIC.SceneGraph.FileReader = function(scene, title, suggestedFileName) {
  var path = scene.IO.queryUserFileAndFolderHandle(scene.IO.forOpen, title, "json", suggestedFileName);

  this.read = function(callback) {
    callback(JSON.parse(scene.IO.getTextFile(path)));
  }
};

/**
 * Constructor to create a FileReader object.
 * @constructor
 * @param {string} filepath The path to the file to read from.
 */
FABRIC.SceneGraph.XHRReader = function(url) {
  
  this.getUrl = function() {
    return url;
  }
  this.read = function(callback) {
    var file = FABRIC.loadResourceURL(url, 'text/JSON', function(fileData){
      callback(JSON.parse(fileData));
    });
  }
};


/*
FABRIC.SceneGraph.registerParser('fez', function(scene, assetUrl, options) {
  options.url = assetUrl;
  return scene.constructNode('LoadBinaryDataNode', options);
});
*/


FABRIC.SceneGraph.registerNodeType('LoadBinaryDataNode', {
  briefDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure files.',
  detailedDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure. ' +
                ' It utilizes a C++ based extension as well as zlib.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      secureKey: undefined,
      version: 1
    });

    var loadBinaryDataNode = scene.constructNode('ResourceLoad', options),
      resourceloaddgnode = loadBinaryDataNode.getDGLoadNode();
    
    loadBinaryDataNode.pub.getResourceFromFile = resourceloaddgnode.getResourceFromFile;

    resourceloaddgnode.addMember('container', 'SecureContainer');
    resourceloaddgnode.addMember('elements', 'SecureElement[]');
    resourceloaddgnode.addMember('secureKey', 'String', options.secureKey);
    
    
    
    resourceloaddgnode.bindings.append(scene.constructOperator({
      operatorName: 'secureContainerLoad',
      parameterLayout: [
        'self.url',
        'self.resource',
        'self.secureKey',
        'self.container',
        'self.elements'
      ],
      entryFunctionName: 'secureContainerLoad',
      srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadSecure.kl',
      async: false,
      mainThreadOnly: true
    }));
    
    var dataTOC = {};
    loadBinaryDataNode.pub.addOnLoadSuccessCallback(function(pub) {
      // define the new nodes based on the identifiers in the file
      resourceloaddgnode.evaluate();
      var elements = resourceloaddgnode.getData('elements',0);
      
      // first, parse for slicecounts
      for(var i=0;i<elements.length;i++) {
        var tokens = elements[i].name.split('.');
        var sgnodeName = tokens[0];
        var dgnodeName = tokens[1];
        var memberName = tokens[2];
        
        if(!dataTOC[sgnodeName]){
          dataTOC[sgnodeName] = {};
        }
        if(!dataTOC[sgnodeName][dgnodeName]){
          dataTOC[sgnodeName][dgnodeName] = {};
          dataTOC[sgnodeName][dgnodeName].sliceCount = elements[i].slicecount;
          dataTOC[sgnodeName][dgnodeName].sliceCountElementIndex = i;
        }
        if(!dataTOC[sgnodeName][dgnodeName][memberName]){
          dataTOC[sgnodeName][dgnodeName][memberName] = elements[i];
        }
        if(dataTOC[sgnodeName][dgnodeName][memberName].first == undefined){
          dataTOC[sgnodeName][dgnodeName][memberName].first = i;
        }
        dataTOC[sgnodeName][dgnodeName][memberName].last = i;
      }
    });
    
    var loadedNodes = {};
    var opSrc = {};
    loadBinaryDataNode.pub.loadDGNodes = function(sgnodeName, dgnodes) {
      
      var sgnodeDataTOC = dataTOC[sgnodeName];
      if(!sgnodeDataTOC){
        console.warn("No data stored for " + sgnodeName);
      }
      
      if(loadedNodes[sgnodeName]){
        console.log("Node already persisted");
        return;
      }
      loadedNodes[sgnodeName] = dgnodes;
      
      var binaryLoadMetadataDGNode = loadBinaryDataNode.constructDGNode(sgnodeName+'MetaDataDGNode');
      
      for(var dgnodeName in dgnodes) {
        
        var dgnode = dgnodes[dgnodeName].dgnode;
        var dgnodeDataToc = sgnodeDataTOC[dgnodeName];
        
        dgnode.setDependency(binaryLoadMetadataDGNode,'metaData');
        dgnode.setDependency(resourceloaddgnode,'secureStorage');
        
        var appendedOps = 0;
        // check if we need to resize this node
        if(dgnodeDataToc.sliceCount > 1) {
          
          binaryLoadMetadataDGNode.addMember(dgnodeName+'sliceCountElementIndex','Integer',dgnodeDataToc.sliceCountElementIndex);
          dgnode.bindings.append(scene.constructOperator({
            operatorName: 'secureContainerResize',
            parameterLayout: [
              'self.newCount',
              'metaData.'+dgnodeName+'sliceCountElementIndex',
              'secureStorage.elements'
            ],
            entryFunctionName: 'secureContainerResize',
            srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadSecure.kl',
            async: false,
            mainThreadOnly: true
          }));
          appendedOps++;
        }
        
        // check if the member exists
        var dgnodeMembers = dgnode.getMembers();
        
        for(var membername in dgnodeDataToc) {
          if(membername== 'sliceCount' || membername== 'sliceCountElementIndex'){
            continue;
          }
          if(!dgnodeMembers[membername]){
            console.warn("SecureStorage: missing member '"+membername+"'.")
            continue;
          }
          var memberDataDesc = dgnodeDataToc[membername];
          // check if the member has the right type
          var originalType = dgnodeMembers[membername].type;
          var type = memberDataDesc.type;
          var isArray = type.indexOf('[]') > -1;
          var isString = type.indexOf('String') > -1;
          var storedType = originalType.replace('Size','Integer');
          if(storedType != type) {
            console.warn('SecureStorage: Member '+membername+' has wrong type: '+originalType);
            continue;
          }
          
          binaryLoadMetadataDGNode.addMember(dgnodeName+membername+'_first','Integer',memberDataDesc.first);
          binaryLoadMetadataDGNode.addMember(dgnodeName+membername+'_last','Integer',memberDataDesc.last);
          
          // create the operator to load it
          var operatorName = 'restoreSecureElement'+originalType.replace('[]','');
          if(isArray){
            operatorName += 'Array';
          }
          if(!opSrc[operatorName]){
            var srcCode = 'use FabricSECURE;\noperator '+operatorName+'(';
            srcCode += '  io SecureContainer container,\n';
            srcCode += '  io SecureElement elements[],\n';
            srcCode += '  io Integer first,\n';
            srcCode += '  io Integer last,\n';
            srcCode += '  io '+originalType.replace('[]','')+' member<>'+(isArray ? '[]' : '')+'\n';
            srcCode += ') {\n';
            if(isArray || memberDataDesc.first != memberDataDesc.last) {
              srcCode += '  Integer outIndex = 0;\n';
              srcCode += '  for(Integer i=first;i<=last;i++) {\n';
              if(storedType != originalType) {
                srcCode += '    '+storedType.replace('[]','')+' memberArray[];\n';
                srcCode += '    memberArray.resize(elements[i].datacount);\n';
                srcCode += '    member[outIndex].resize(elements[i].datacount);\n';
                srcCode += '    container.getElementData(i,memberArray.data(),memberArray.dataSize());\n'
                srcCode += '    for(Size j=0;j<elements[i].datacount;j++) {\n';
                srcCode += '      member[i][j] = '+originalType.replace('[]','')+'(memberArray[j]);\n';
                srcCode += '    }\n';
              } else {
                srcCode += '    member[outIndex].resize(elements[i].datacount);\n';
                srcCode += '    container.getElementData(i,member[outIndex].data(),member[outIndex].dataSize());\n'
              }
              srcCode += '    outIndex++;\n'
              srcCode += '  }\n'
            }
            else if(isString) {
              srcCode += '  Integer outIndex = 0;\n';
              srcCode += '  for(Integer i=first;i<=last;i++) {\n';
              srcCode += '    Size length = Size(elements[i].datacount);\n';
              srcCode += '    member[outIndex] = "";\n';
              srcCode += '    while(length > 0) {\n';
              srcCode += '      if(length > 9) {\n';
              srcCode += '        member[outIndex] += "0000000000";\n';
              srcCode += '        length -= 10;\n';
              srcCode += '      } else {\n';
              srcCode += '        member[outIndex] += "0";\n';
              srcCode += '        length -= 1;\n';
              srcCode += '      }\n';
              srcCode += '    }\n';
              srcCode += '    container.getElementData(i,member[outIndex].data(),member[outIndex].length());\n'
              srcCode += '    outIndex++;\n'
              srcCode += '  }\n'
            }
            else {
              if(storedType != originalType) {
                srcCode += '  '+storedType.replace('[]','')+' memberArray[];\n';
                srcCode += '  memberArray.resize(member.size());\n';
                srcCode += '  container.getElementData(first,memberArray.data(),memberArray.dataSize());\n'
                srcCode += '  for(Size i=0;i<member.size();i++) {\n';
                srcCode += '    member[i] = '+originalType.replace('[]','')+'(memberArray[i]);\n';
                srcCode += '  }\n';
              } else {
                srcCode += '  container.getElementData(first,member.data(),member.dataSize());\n'
              }
            }
            srcCode += '}\n';
            opSrc[operatorName] = srcCode;
          }
          dgnode.bindings.append(scene.constructOperator({
              operatorName: operatorName,
              srcCode: opSrc[operatorName],
              entryFunctionName: operatorName,
              parameterLayout: [
                'secureStorage.container',
                'secureStorage.elements',
                'metaData.'+dgnodeName+membername+'_first',
                'metaData.'+dgnodeName+membername+'_last',
                'self.'+membername+'<>'
              ],
              async: false,
              mainThreadOnly: true
            }));
          appendedOps++;
        }
        loadedNodes[sgnodeName][dgnodeName].appendedOps = appendedOps;
      }
    };
    
    loadBinaryDataNode.disposeData = function(){
      /*
      for(var sgnodename in loadedNodes){
        for(var dgnodename in loadedNodes[sgnodename]){
          var dgnodeData = loadedNodes[sgnodename][dgnodename];
          var dgnode = dgnodeData.dgnode;
          dgnode.evaluate();
          var numOps = dgnode.bindings.getLength();
          for(var i=numOps-1; i>=numOps-dgnodeData.appendedOps; i--){
            dgnode.bindings.remove(i);
          }
        }
      }
      
      // this frees up the memory used by the resource.
      resourceloaddgnode.removeMember('resource');
      resourceloaddgnode.removeMember('container');
      resourceloaddgnode.removeMember('elements');
      */
    }
    return loadBinaryDataNode;
  }
});


FABRIC.SceneGraph.registerNodeType('WriteBinaryDataNode', {
  briefDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure files.',
  detailedDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure. It utilizes a C++ based extension as well as zlib.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      compressionLevel: 1, // 0 - 9
      secureKey: undefined
    });
      
    var writeBinaryDataNode = scene.constructNode('SceneGraphNode', options);
    
    var writeBinaryDataNodeEvent = writeBinaryDataNode.constructEventNode('WriteBinaryEvent');
    var binarydatadgnode = writeBinaryDataNode.constructResourceLoadNode('DGLoadNode');
    var persistedNodes = {};
    var eventHandlers = {};
    
    writeBinaryDataNode.pub.write = function(resource, path){
      var dgErrors = writeBinaryDataNodeEvent.getErrors();
      if(dgErrors.length > 0){
        throw dgErrors;
      }
      for (var ehname in eventHandlers) {
        if (eventHandlers.hasOwnProperty(ehname)) {
          var dgErrors = eventHandlers[ehname].getErrors();
          if (dgErrors.length > 0) {
            errors.push(ehname + ':' + JSON.stringify(dgErrors));
          }
        }
      }
      
      writeBinaryDataNodeEvent.fire();
      binarydatadgnode.putResourceToFile(resource, path);
    }
    
    binarydatadgnode.addMember('container', 'SecureContainer');
    binarydatadgnode.addMember('elements', 'SecureElement[]');
    binarydatadgnode.addMember('secureKey', 'String', options.secureKey);
    binarydatadgnode.addMember('compressionLevel', 'Integer', options.compressionLevel);

    var writeEventHandler = writeBinaryDataNode.constructEventHandlerNode('Propagation');
    writeEventHandler.setScope('container', binarydatadgnode);
    // attach an operator to clear the container!
    var nbOps = 0;
    var opSrc = {};
    writeEventHandler.preDescendBindings.append(scene.constructOperator({
      operatorName: 'secureContainerClear',
      parameterLayout: [
        'container.container',
        'container.elements'
      ],
      entryFunctionName: 'secureContainerClear',
      srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadSecure.kl',
      async: false,
      mainThreadOnly: true
    }));
    nbOps++;
    
    writeEventHandler.postDescendBindings.append(scene.constructOperator({
      operatorName: 'secureContainerSave',
      parameterLayout: [
        'container.container',
        'container.resource',
        'container.compressionLevel',
        'container.secureKey'
      ],
      entryFunctionName: 'secureContainerSave',
      srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadSecure.kl',
      async: false,
      mainThreadOnly: true
    }));
    nbOps++;
    
    writeBinaryDataNodeEvent.appendEventHandler(writeEventHandler);
    eventHandlers['writeEventHandler'] = writeEventHandler;
    
    // methods to store nodes
    writeBinaryDataNode.storeDGNodes = function(sgnodeName, dgnodeDescs) {
      if(persistedNodes[sgnodeName]){
        console.log("Node already persisted");
        return;
      }
      persistedNodes[sgnodeName] = dgnodeDescs;
      var writeDGNodesEventHandler = writeBinaryDataNode.constructEventHandlerNode('Write'+sgnodeName);
      writeEventHandler.appendChildEventHandler(writeDGNodesEventHandler);
      eventHandlers['Write'+sgnodeName] = writeDGNodesEventHandler;
      for(dgnodeName in dgnodeDescs){
        var dgnode = dgnodeDescs[dgnodeName].dgnode;
        var memberNames = dgnodeDescs[dgnodeName].members;
        var members = dgnode.getMembers();
        if(!memberNames) {
          memberNames = [];
          for(var name in members){
            memberNames.push(name);
          }
        }
        
        writeDGNodesEventHandler.setScope(dgnodeName, dgnode);
        
        // now setup the operators to store the data
        for(var i=0;i<memberNames.length;i++) {
          var memberName = memberNames[i];
          var memberType = members[memberName].type;
          
          var isArray = memberType.indexOf('[]') > -1;
          var isString = memberType.indexOf('String') > -1;
          if(isArray && isString){
            console.log("SecureStorage: Warning: Skipping member '"+memberName+"'. String Array members are not supported at this stage.");
            continue;
          }
          
          var operatorName = 'storeSecureElement'+memberType.replace('[]','');
          if(isArray)
            operatorName += 'Array';
          
          // We currenlty store 'Size' values as 'Integer' values to be platform neutral.
          var storedType = memberType.replace('Size','Integer');
  
          writeDGNodesEventHandler.addMember(memberName+'_name','String', sgnodeName+'.'+dgnodeName+'.'+memberName);
          writeDGNodesEventHandler.addMember(memberName+'_type','String',storedType);
          writeDGNodesEventHandler.addMember(memberName+'_version','Integer',options.version);
          
          if(!opSrc[operatorName]){
            var srcCode = 'use FabricSECURE;\noperator '+operatorName+'(\n';
            srcCode += '  io SecureContainer container,\n';
            srcCode += '  io SecureElement elements[],\n';
            srcCode += '  io String memberName,\n';
            srcCode += '  io String memberType,\n';
            srcCode += '  io Integer memberVersion,\n';
            srcCode += '  io '+memberType.replace('[]','')+' member<>'+(isArray ? '[]' : '')+'\n';
            srcCode += ') {\n';
            srcCode += '  SecureElement element;\n';
            srcCode += '  element.name = memberName;\n';
            srcCode += '  element.type = memberType;\n';
            srcCode += '  element.version = memberVersion;\n';
            srcCode += '  element.slicecount = member.size();\n';
            if(isArray) {
              srcCode += '  for(Size i=0;i<member.size();i++){\n';
              srcCode += '    element.sliceindex = Integer(i);\n';
              srcCode += '    element.datacount = member[i].size();\n';
              if(storedType != memberType) {
                srcCode += '    '+storedType.replace('[]','')+' memberArray[];\n';
                srcCode += '    memberArray.resize(member[i].size());\n';
                srcCode += '    for(Size j=0;j<member[i].size();j++) {\n';
                srcCode += '      memberArray[j] = '+storedType.replace('[]','')+'(member[i][j]);\n';
                srcCode += '    }\n';
                srcCode += '    container.addElement(element,memberArray.data(),memberArray.dataSize());\n'
              } else {
                srcCode += '    container.addElement(element,member[i].data(),member[i].dataSize());\n'
              }
              srcCode += '  }\n'
            } else if(isString) {
              srcCode += '  for(Size i=0;i<member.size();i++){\n';
              srcCode += '    element.sliceindex = Integer(i);\n';
              srcCode += '    element.datacount = member[i].length();\n';
              srcCode += '    container.addElement(element,member[i].data(),member[i].length());\n'
              srcCode += '  }\n'
            } else {
              srcCode += '  element.sliceindex = -1;\n';
              srcCode += '  element.datacount = member.size();\n';
              if(storedType != memberType) {
                srcCode += '  '+storedType.replace('[]','')+' memberArray[];\n';
                srcCode += '  memberArray.resize(member.size());\n';
                srcCode += '  for(Size i=0;i<member.size();i++) {\n';
                srcCode += '    memberArray[i] = '+storedType.replace('[]','')+'(member[i]);\n';
                srcCode += '  }\n';
                srcCode += '  container.addElement(element,memberArray.data(),memberArray.dataSize());\n'
              } else {
                srcCode += '  container.addElement(element,member.data(),member.dataSize());\n'
              }
            }
            srcCode += '  elements.push(element);\n';
            srcCode += '}\n';
            opSrc[operatorName] = srcCode;
          }
          writeDGNodesEventHandler.preDescendBindings.append(scene.constructOperator({
            operatorName: operatorName,
            srcCode: opSrc[operatorName],
            entryFunctionName: operatorName,
            parameterLayout: [
              'container.container',
              'container.elements',
              'self.'+memberName+'_name',
              'self.'+memberName+'_type',
              'self.'+memberName+'_version',
              dgnodeName+'.'+memberName+'<>'
            ],
            async: false
          }));
        }
      }
    };
    
    return writeBinaryDataNode;
  }
});

