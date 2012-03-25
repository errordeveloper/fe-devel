/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Particles"], function() {
  
// The Particles node is a relative of the 'Redraw' node.
// It is bound to a 'Geometry' which it renders for each Instance.

// Particles is similar to 'Points' except it is set up for multi-threading.
FABRIC.SceneGraph.registerNodeType('Crowd', {
  briefDesc: '',
  detailedDesc: '',
  parentNodeDesc: 'LocomotionCharacterController',
  optionsDesc: {
    cellsize: 'The cell size for the optional spatial hash table',
    x_count: 'The count of the segments along the X axis for the optional grid node.',
    y_count: 'The count of the segments along the Y axis for the optional grid node.',
    z_count: 'The count of the segments along the Z axis for the optional grid node.',
    displayGrid: 'Optionally create a grid node to create a reference for the Particles node.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      cellsize: 5.0,
      x_count: 30,
      y_count: 1,
      z_count: 30,
      displayGrid: true,
      agentCount: 3,
      agentRadius: (this.cellsize*0.3),
      xfos: undefined,
      displayDebugging: false,
      enableDebugging: false
    });
    
    var crowdNode = scene.constructNode('LocomotionCharacterController', options);
    var dgnode = crowdNode.getDGNode();
    var hashtablenode = crowdNode.constructDGNode('hashtablenode');
    dgnode.setDependency(hashtablenode, 'hashtable');
    
    var hashTable = FABRIC.RT.hashTable(options.cellsize, options.x_count, options.y_count, options.z_count);
    hashtablenode.addMember('hashtable', 'HashTable', hashTable);
    
    dgnode.addMember('cellindex', 'Integer', -1 );
    dgnode.addMember('cellcoord', 'Vec3');
    dgnode.addMember('previousframe_position', 'Vec3');
    dgnode.addMember('previousframe_velocity', 'Vec3');

    // Display the Grid
    if (options.displayGrid){
      scene.pub.constructNode('Instance', {
        geometryNode: scene.pub.constructNode('Grid', {
          size_x: options.cellsize * options.x_count,
          size_z: options.cellsize * options.z_count,
          sections_x: options.x_count + 1,
          sections_z: options.z_count + 1
        }),
        materialNode: scene.pub.constructNode('FlatMaterial', { color: FABRIC.RT.rgb(0.2, 0.2, 0.2) })
      });
    }


    // Swap buffers. This means that the up to date buffer is now the back buffer
    // and we will calculate the front buffer this update.
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'copyCurrentFrameDataToPrevFrameData',
      srcFile: 'FABRIC_ROOT/SG/KL/crowds.kl',
      entryPoint: 'copyCurrentFrameDataToPrevFrameData',
      parameterLayout: [
        'self.xfo',
        'self.linearVelocity',
        'self.previousframe_position',
        'self.previousframe_velocity',
        'hashtable.hashtable'
      ]
    }));
    
    
    // Calculate our cell index based on the back buffer data.
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'calcCellIndex',
      srcFile: 'FABRIC_ROOT/SG/KL/crowds.kl',
      entryPoint: 'calcCellIndex',
      parameterLayout: [
        'self.previousframe_position',
        'self.cellcoord',
        'self.cellindex',
        'hashtable.hashtable'
      ]
    }));

    // This operator counts all the points which exist in each
    // cell and writes thier ids to the hash table. This operator
    // is single threaded and should be the last to execute.
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'populateHashTable',
      srcFile: 'FABRIC_ROOT/SG/KL/crowds.kl',
      entryPoint: 'populateHashTable',
      parameterLayout: [
        'hashtable.hashtable',
        'self.cellindex<>'
      ]
    }));
    
    
    dgnode.addMember('goal', 'Vec3');
    dgnode.addMember('neighborIndices', 'Integer[]');
    dgnode.addMember('neighborDistances', 'Scalar[]');
    
    var neighborInfluenceRange = options.cellsize / 2.0;
    dgnode.addMember('neighborinfluencerange', 'Scalar', neighborInfluenceRange );
    dgnode.addMember('agentRadius', 'Scalar', options.agentRadius );
    
    dgnode.addMember('initialized', 'Boolean', false );
    dgnode.addMember('debugCrowd', 'DebugGeometry' );
    hashtablenode.addMember('displayDebugging', 'Boolean', options.displayDebugging );
    crowdNode.addMemberInterface(hashtablenode, 'displayDebugging', true);
    
    dgnode.setDependency(scene.getGlobalsNode(), 'globals');
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'simulateCrowd',
      srcFile: 'FABRIC_ROOT/SG/KL/crowds.kl',
      entryPoint: 'simulateCrowd',
      parameterLayout: [
        'self.index',
        'self.initialized',

        'self.goal',
        'self.cellindex',
        'self.cellcoord',

        'self.xfo',
        'self.goalLinearVelocity',
        'self.previousframe_position<>',
        'self.previousframe_velocity<>',
        'self.maxLinearVelocity',
        'self.maxAngularVelocity',
        
        'self.linearVelocity',

        'self.neighborinfluencerange',
        'self.agentRadius',
        'hashtable.hashtable',

        'globals.timestep',

        'self.neighborIndices',
        'self.neighborDistances',
        'self.debugCrowd',
        'hashtable.displayDebugging'
      ]
    }));
    
    
    var debugGeometryDraw = scene.constructNode('DebugGeometryDraw', {
        dgnode: dgnode,
        debugGemetryMemberName: 'debugCrowd'
    });
    
    // We always append fht econtrolelr op last, 
    // because it increments the character Xfo
    dgnode.bindings.append(crowdNode.getCharacterControllerOp());
    
    crowdNode.pub.setCrowdXfos = function(xfos){
      dgnode.resize(xfos.length);
      dgnode.setBulkData({ xfo: xfos});
    }
    
    crowdNode.pub.setCrowdGoals = function(goals){
      dgnode.resize(goals.length);
      dgnode.setBulkData({ goal: goals});
    }
    
    if(!options.xfos){
      options.xfos = [];
      var size_x = options.cellsize * options.x_count;
      var size_z = options.cellsize * options.z_count;
      for(var i=0; i<options.agentCount; i++){
        options.xfos.push(new FABRIC.RT.Xfo({tr:new FABRIC.RT.Vec3((Math.random()-0.5)*size_x, 0,(Math.random()-0.5)*size_z)}));
      }
    }
    crowdNode.pub.setCrowdXfos(options.xfos);

    return crowdNode;
  }});


});
