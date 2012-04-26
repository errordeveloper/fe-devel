/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph"], function() {

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
          var nodeData = { };
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
                  str += '\n          \"sliceCount\":' + dgnode.size();
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
          }
          str += '\n  ]';
          str += '\n}';
          writer.write(str);
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
      preLoadScene: false,
      filteredNodeTypes: [],
      typeRemappings: {}
    });
    
    // Preloading nodes enables data to be loaded into existing nodes.
    // This is usefull when saving presets, rather then scenegraph descriptions.
    var preLoadedNodes = {};
    var dataObj;
  
    var constructedNodeMap = {};
    var nodeNameToStoredNameRemapping = {};
    var nodeStoredNameToNodeRemapping = {};
    var nodeDataMap = {};
    
    if(options.preLoadScene){
      var sceneNodes = scene.getSceneGraphNodes();
      for(var name in sceneNodes){
        preLoadedNodes[name] = sceneNodes[name].pub;
      }
    };
    var nodeData;
    var sgnodeDataMap = {};
    
    
    var sceneDeserializer = {
      getNode: function(storedNodeName) {
        var nodeName = nodeStoredNameToNodeRemapping[ storedNodeName ]
        if (constructedNodeMap[nodeName]) {
          return constructedNodeMap[nodeName];
        }else {
          return scene.pub.getSceneGraphNode(nodeName);
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
            if((typeof data)=='string'){
              data = JSON.parse(data);
            }
            dataObj = data;
            
            var loadNodes = function(){
              // Assign the function for interfacing with the binary node.
              // by assigning the function here, the closure contains the binary data node.
              var sceneDeserializerInterface = {
                getNode: sceneDeserializer.getNode,
                loadDGNodesData: function(sgnodeName, desc) {
                  sgnodeName = nodeNameToStoredNameRemapping[sgnodeName];
                  if(dataObj.metadata.binaryStorage){
                    loadNodeBinaryFileNode.pub.addEventListener('loadSuccess', function(){
                      loadNodeBinaryFileNode.pub.loadDGNodes(sgnodeName, desc);
                      return;
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
                      dgnode.resize(data.sliceCount);
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
                }
              }
                
              var remainingNodes = dataObj.sceneGraphNodes.length;
              var loadDGNode = function(nodeData){
                nodeDataMap[nodeData.name] = nodeData;
                FABRIC.createAsyncTask(function(){
                  
                  for(var i=0; i<options.filteredNodeTypes.length; i++){
                    if (options.filteredNodeTypes[i] == nodeData.type) {
                      return;
                    }
                  }
                  var node = preLoadedNodes[nodeData.name];
                  if (!node) {
                    var type = options.typeRemappings[nodeData.type] || nodeData.type;
                    node = scene.pub.constructNode(type, nodeData.options);
                  }
                  // in case a name collision occured, store a name remapping table.
                  nodeStoredNameToNodeRemapping[ nodeData.name ] = node.getName();
                  nodeNameToStoredNameRemapping[ node.getName() ] = nodeData.name;
                  var nodePrivate = scene.getPrivateInterface(node);
                  nodePrivate.readData(sceneDeserializerInterface, nodeData.data);
                  constructedNodeMap[node.getName()] = node;
                  remainingNodes--;
                  if(remainingNodes == 0){
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
              
            }
            
            
            if(dataObj.metadata.binaryStorage){
              
              var binaryFilePath;
              if(storage.getUrl){
                var pathArray = storage.getUrl().split('/');
                pathArray.pop();
                if(pathArray.length == 0){
                  binaryFilePath = dataObj.metadata.binaryFilePath;
                }else{
                  binaryFilePath = pathArray.join('/') + '/' + dataObj.metadata.binaryFilePath;
                }
              }
              var loadNodeBinaryFileNode = scene.constructNode('LoadBinaryDataNode', {
                url: binaryFilePath
              });
              
              loadNodeBinaryFileNode.pub.addEventListener('loadSuccess', function(){
                // Construct the graph only once the binary node has been loaded.
                loadNodes();
              });
            }else{
              
              loadNodes();
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
FABRIC.SceneGraph.FileWriter = function(scene, title, suggestedFileName, fileType) {
  
  var path;
  var str = "";
  path = FABRIC.IO.queryUserFileAndFolderHandle(FABRIC.IO.forSave, title, fileType ? fileType : "json", suggestedFileName);
  
  this.write = function(instr) {
    str = instr;
    FABRIC.IO.putTextFileContent(path.file, str);
  }
  this.log = function() {
    console.log(str);
  }
};

FABRIC.SceneGraph.FileWriterWithBinary = function(scene, title, suggestedFileName, options) {
  
  var jsonfilehandle = FABRIC.IO.queryUserFileAndFolderHandle(FABRIC.IO.forSave, title, "json", suggestedFileName);
  var jsonfilename = FABRIC.IO.getFileHandleInfo( jsonfilehandle.file ).fileName;
  var binarydatafilehandle = FABRIC.IO.buildFileHandleFromRelativePath(jsonfilehandle.folder+'/'+jsonfilename.split('.')[0]+'.feb');
  
  var writeBinaryDataNode = scene.constructNode('WriteBinaryDataNode', options);
      
  var str = "";
  this.getBinaryStorageNode = function(){
    return writeBinaryDataNode;
  }
  
  this.getBinaryFileName = function(){
    return FABRIC.IO.getFileHandleInfo( binarydatafilehandle ).fileName;
  }
  
  this.write = function(instr) {
    str = instr;
    FABRIC.IO.putTextFileContent(jsonfilehandle.file, str);
    writeBinaryDataNode.write(binarydatafilehandle);
  }
  this.writeJSON = function(instr) {
    str = instr;
    FABRIC.IO.putTextFileContent(path.file, str);
  }
  
  this.writeBinary = function() {
    writeBinaryDataNode.write(binarydatapath.file, str);
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
  var path = FABRIC.IO.queryUserFileAndFolderHandle(FABRIC.IO.forOpen, title, "json", suggestedFileName);

  this.read = function(callback) {
    callback(JSON.parse(FABRIC.IO.getTextFile(path)));
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



FABRIC.SceneGraph.registerNodeType('LoadBinaryDataNode', {
  briefDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure files.',
  detailedDesc: 'The SecureStorageNode node is a ResourceLoad node able to load or save Fabric Engine Secure. ' +
                ' It utilizes a C++ based extension as well as zlib.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
    });

    options.storeDataAsFile = true;
    var loadBinaryDataNode = scene.constructNode('ResourceLoad', options),
      resourceloaddgnode = loadBinaryDataNode.getDGLoadNode();
    
    resourceloaddgnode.addMember('stream','FabricFileStream');
    resourceloaddgnode.addMember('dataNames','String[]');
    resourceloaddgnode.addMember('seekOffsets','Size[]');
    // Preconstruct the operator in advance to avoid the bug in chrome.
    
      resourceloaddgnode.bindings.append(scene.constructOperator({
        operatorName: 'readTOCFromStream',
        parameterLayout: [
          "self.resource",
          "self.dataNames",
          "self.seekOffsets"
        ],
        entryPoint: 'readTOCFromStream',
        srcFile: 'FABRIC_ROOT/SG/KL/fileStream.kl',
        async: false
      }));
    
    var dataTOC = {};
    loadBinaryDataNode.pub.addEventListener('loadSuccess', function(pub) {
      // define the new nodes based on the identifiers in the file
      resourceloaddgnode.bindings.append(scene.constructOperator({
        operatorName: 'readTOCFromStream',
        parameterLayout: [
          "self.resource",
          "self.dataNames",
          "self.seekOffsets"
        ],
        entryPoint: 'readTOCFromStream',
        srcFile: 'FABRIC_ROOT/SG/KL/fileStream.kl',
        async: false
      }));
      resourceloaddgnode.evaluate();
      var dataNames = resourceloaddgnode.getData('dataNames',0);
      
      // first, parse for slicecounts
      for(var i=0;i<dataNames.length;i++) {
        var tokens = dataNames[i].split('.');
        var sgnodeName = tokens[0];
        var dgnodeName = tokens[1];
        
        if(!dataTOC[sgnodeName]){
          dataTOC[sgnodeName] = {};
        }
        if(!dataTOC[sgnodeName][dgnodeName]){
          dataTOC[sgnodeName][dgnodeName] = {};
        }
        if(tokens.length == 3){
          var memberName = tokens[2];
          if(!dataTOC[sgnodeName][dgnodeName][memberName]){
            dataTOC[sgnodeName][dgnodeName][memberName] = { index:i };
          }
        }
      }
    });
    
    loadBinaryDataNode.pub.loadDGNodes = function(sgnodeName, dgnodes) {
      
      var sgnodeDataTOC = dataTOC[sgnodeName];
      if(!sgnodeDataTOC){
        console.warn("No data stored for " + sgnodeName);
        return;
      }
      
      var binaryLoadMetadataDGNode = loadBinaryDataNode.constructDGNode(sgnodeName+'MetaDataDGNode');
      binaryLoadMetadataDGNode.addMember('prevDataIndex','Integer');
      
      for(var dgnodeName in dgnodes) {
        
        var dgnode = dgnodes[dgnodeName].dgnode;
        var dgnodeDataToc = sgnodeDataTOC[dgnodeName];
        
        dgnode.setDependency(binaryLoadMetadataDGNode,'metaData');
        dgnode.setDependency(resourceloaddgnode,'resourceNode');
        
        var dataName = sgnodeName+'.'+dgnodeName;
        binaryLoadMetadataDGNode.addMember(dgnodeName+'sliceCount','String',dataName);
        
        dgnode.bindings.append(scene.constructOperator({
          operatorName: 'readSliceCountFromStream',
          parameterLayout: [
            'resourceNode.resource',
            'resourceNode.dataNames',
            'resourceNode.seekOffsets',
            'metaData.'+dgnodeName+'sliceCount',
            'self'
          ],
          preProcessorDefinitions: {
            DATA_TYPE: 'Vec3'
          },
          entryPoint: 'readSliceCountFromStream',
          srcFile: 'FABRIC_ROOT/SG/KL/fileStreamIO.kl'
        }));
        
        // check if the member exists
        var dgnodeMembers = dgnode.getMembers();
        
        for(var memberName in dgnodeDataToc) {
          if(!dgnodeMembers[memberName]){
            console.warn("SecureStorage: missing member '"+memberName+"'.")
            continue;
          }
          var memberDataDesc = dgnodeDataToc[memberName];
          // check if the member has the right type
          var memberType = dgnodeMembers[memberName].type;
          var isArray = memberType.indexOf('[]') > -1;
          if(isArray){
            memberType = memberType.slice(0, memberType.length-2);
          }
          var isString = memberType.indexOf('String') > -1;
          
          var dataName = sgnodeName+'.'+dgnodeName+'.'+memberName;
          binaryLoadMetadataDGNode.addMember(memberName+'_name','String', dataName);
          var operatorName = 'read' + (isArray ? 'Array' : 'Member') + 'FromStream';
          
          dgnode.bindings.append(scene.constructOperator({
            operatorName: operatorName + memberType,
            srcFile: 'FABRIC_ROOT/SG/KL/fileStreamIO.kl',
            preProcessorDefinitions: {
              DATA_TYPE: memberType
            },
            entryPoint: operatorName,
            parameterLayout: [
              'resourceNode.resource',
              'resourceNode.dataNames',
              'resourceNode.seekOffsets',
              'metaData.'+memberName+'_name',
              'self.'+memberName+'<>'
            ]
          }));
        }
      }
    };
    
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
    });
      
    var writeBinaryDataNode = scene.constructNode('SceneGraphNode', options);
    
    var writeBinaryDataNodeEvent = writeBinaryDataNode.constructEventNode('WriteBinaryEvent');
    var binarydatadgnode = writeBinaryDataNode.constructResourceLoadNode('DGLoadNode');
    var persistedNodes = {};
    var eventHandlers = {};
    var dataNames = [];
    
    writeBinaryDataNode.pub.write = function(handle){
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
      
      writeEventHandler.setData('handle', 0, handle);
      writeEventHandler.setData('dataNames', 0, dataNames);
      // when this event fires, it writes the data to the file on disk.
      writeBinaryDataNodeEvent.fire();
    }
    var writeEventHandler = writeBinaryDataNode.constructEventHandlerNode('Propagation');
    
    writeEventHandler.addMember('handle','String');
    writeEventHandler.addMember('stream','FabricFileStream');
    writeEventHandler.addMember('dataNames','String[]');
    writeEventHandler.addMember('seekOffsetsLocation','Size');
    writeEventHandler.addMember('seekOffsets','Size[]');
    

    writeEventHandler.setScopeName('fileStream');
    
    // attach an operator to clear the container!
    writeEventHandler.preDescendBindings.append(scene.constructOperator({
      operatorName: 'openFileStreamForWriting',
      parameterLayout: [
        "self.handle",
        "self.stream",
        "self.dataNames",
        "self.seekOffsetsLocation",
        "self.seekOffsets"
      ],
      entryPoint: 'openFileStreamForWriting',
      srcFile: 'FABRIC_ROOT/SG/KL/fileStream.kl',
      async: false
    }));
    
    writeEventHandler.postDescendBindings.append(scene.constructOperator({
      operatorName: 'closeFileStream',
      parameterLayout: [
        "self.handle",
        "self.stream",
        "self.seekOffsetsLocation",
        "self.seekOffsets"
      ],
      entryPoint: 'closeFileStream',
      srcFile: 'FABRIC_ROOT/SG/KL/fileStream.kl',
      async: false
    }));
    
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
        var dataName = sgnodeName+'.'+dgnodeName;
        writeDGNodesEventHandler.addMember(dataName,'String', dataName);
        
        writeDGNodesEventHandler.preDescendBindings.append(scene.constructOperator({
          operatorName: 'writeSliceCountToStream',
          srcFile: 'FABRIC_ROOT/SG/KL/fileStreamIO.kl',
          preProcessorDefinitions: {
            DATA_TYPE: 'Size'
          },
          entryPoint: 'writeSliceCountToStream',
          parameterLayout: [
            'fileStream.stream',
            'fileStream.dataNames',
            'fileStream.seekOffsets',
            'self.'+dataName,
            dgnodeName
          ],
          async: false
        }));
          
        dataNames.push(dataName);
        // now setup the operators to store the data
        for(var i=0;i<memberNames.length;i++) {
          var memberName = memberNames[i];
          var memberType = members[memberName].type;
          var isArray = memberType.indexOf('[]') > -1;
          if(isArray){
            memberType = memberType.slice(0, memberType.length-2);
          }
          var isString = memberType.indexOf('String') > -1;
          
          var dataName = sgnodeName+'.'+dgnodeName+'.'+memberName;
          writeDGNodesEventHandler.addMember(memberName+'_name','String', dataName);
          var operatorName = 'write'+ memberType + (isArray ? 'Array' : 'Member') + 'ToStream';
          var entryPoint = 'write' + (isArray ? 'Array' : 'Member') + 'ToStream';
          
          writeDGNodesEventHandler.preDescendBindings.append(scene.constructOperator({
            operatorName: operatorName,
            srcFile: 'FABRIC_ROOT/SG/KL/fileStreamIO.kl',
            preProcessorDefinitions: {
              DATA_TYPE: memberType
            },
            entryPoint: entryPoint,
            parameterLayout: [
              'fileStream.stream',
              'fileStream.dataNames',
              'fileStream.seekOffsets',
              'self.'+memberName+'_name',
              dgnodeName+'.'+memberName+'<>'
            ],
            async: false
          }));
          
          dataNames.push(dataName);
        }
      }
    };
    
    return writeBinaryDataNode;
  }
});

});
