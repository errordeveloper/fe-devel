
//
// Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
//

FABRIC.define(["SceneGraph/FABRIC.SceneGraph",
               "SceneGraph/FABRIC.SceneGraph.Geometry"], function() {
  
FABRIC.SceneGraph.registerParser('las', function(scene, assetUrl, options) {
  
  var results = {};
  var assetName = assetUrl.split('/').pop().split('.')[0];
  
  options.url = assetUrl;
  results[options.baseName] = scene.constructNode('LidarLoadNode', options);
  return results;
});

FABRIC.SceneGraph.registerParser('laz', function(scene, assetUrl, options) {
  
  var results = {};
  var assetName = assetUrl.split('/').pop().split('.')[0];
  
  options.url = assetUrl;
  results[options.baseName] = scene.constructNode('LidarLoadNode', options);
  return results;
});

FABRIC.SceneGraph.registerNodeType('LidarLoadNode', {
  briefDesc: 'The LidarLoadNode node is a ResourceLoad node able to parse laslib files.',
  detailedDesc: 'The LidarLoadNode node is a ResourceLoad node able to parse laslib files. It utilizes a C++ based extension and generated parsed nodes such as Points nodes.',
  parentNodeDesc: 'ResourceLoad',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      removeParsersOnLoad: false,
      dependentNode: undefined
    });

    var resourceLoadNode = scene.constructNode('ResourceLoad', options),
      resourceloaddgnode = resourceLoadNode.getDGLoadNode();
      
    // make the dependent node, well, dependent
    if(options.dependentNode != undefined) {
      var priv = scene.getPrivateInterface(options.dependentNode);
      for(var dgnodeName in priv.getDGNodes()) {
        priv.getDGNodes()[dgnodeName].setDependency(resourceloaddgnode,options.url);
      }
    }

    resourceloaddgnode.addMember('lidar', 'LidarReader');
    
    resourceloaddgnode.bindings.append(scene.constructOperator({
      operatorName: 'lidarLoad',
      parameterLayout: [
        'self.url', //For debugging only
        'self.resource',
        'self.lidar'
      ],
      entryFunctionName: 'lidarLoad',
      srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadLidar.kl',
      async: false
    }));
    
    resourceLoadNode.pub.addEventListener('loadSuccess', function(pub) {

      // create the points node
      var pointsNode = scene.constructNode('Points');
      var pointsAttributeDGNode = pointsNode.getAttributesDGNode();
      pointsNode.pub.addVertexAttributeValue('vertexColors','Color',{ genVBO:true });
      pointsAttributeDGNode.setDependency(resourceloaddgnode,'resource');

      pointsAttributeDGNode.bindings.append(scene.constructOperator({
        operatorName: 'lidarGetCount',
        parameterLayout: [
          'resource.lidar',
          'self.newCount'
        ],
        entryFunctionName: 'lidarGetCount',
        srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadLidar.kl',
        async: false
      }));
      
      pointsAttributeDGNode.bindings.append(scene.constructOperator({
        operatorName: 'lidarGetPoints',
        parameterLayout: [
          'resource.lidar',
          'self.positions<>',
          'self.vertexColors<>'
        ],
        entryFunctionName: 'lidarGetPoints',
        srcFile: 'FABRIC_ROOT/SceneGraph/KL/loadLidar.kl',
        async: false
      }));

      var parsedNodes = {points: pointsNode.pub};
      resourceLoadNode.pub.getParsedNodes = function(){
        return parsedNodes;
      }
    });
    
    return resourceLoadNode;
  }
});

});