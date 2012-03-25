/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Geometry",
               "SG/DebugGeometry",
               "RT/HashTable"], function() {

// The Particles node is a relative of the 'Redraw' node.
// It is bound to a 'Geometry' which it renders for each Instance.

// Particles is similar to 'Points' except it is set up for multi-threading.
FABRIC.SceneGraph.registerNodeType('Particles', {
  briefDesc: 'The Particles node is a Points node with additional vertex attributes.',
  detailedDesc: 'The Particles node is a Points node with additional vertex attributes. It can be used to draw ' +
                'simulated Points with full transforms, so the Particles node contains more than only positions, '+
                'also orientations, velocities etc.',
  parentNodeDesc: 'Points',
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
      displayGrid: true
    });

    var particlesNode = scene.constructNode('Points', options);
    
    particlesNode.pub.setAttributeDynamic('positions');
    particlesNode.pub.addVertexAttributeValue('orientations', 'Vec3');
    particlesNode.pub.addVertexAttributeValue('velocities', 'Vec3');

    particlesNode.pub.addUniformValue('hashtable', 'HashTable',
      FABRIC.RT.hashTable(options.cellsize, options.x_count, options.y_count, options.z_count));
    particlesNode.pub.addVertexAttributeValue('cellindices', 'Integer', { defaultValue:-1 });
    particlesNode.pub.addVertexAttributeValue('cellcoords', 'Vec3');

    particlesNode.pub.addVertexAttributeValue('previousframe_positions', 'Vec3');
    particlesNode.pub.addVertexAttributeValue('previousframe_velocities', 'Vec3');
    particlesNode.pub.addVertexAttributeValue('previousframe_orientations', 'Vec3');

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

    // Calculate our cell index based on the back buffer data.
    particlesNode.getAttributesDGNode().bindings.append(scene.constructOperator({
      operatorName: 'calcCellIndex',
      srcFile: 'FABRIC_ROOT/SG/KL/particles.kl',
      entryPoint: 'calcCellIndex',
      parameterLayout: [
        'self.positions',
        'self.cellcoords',
        'self.cellindices',
        'uniforms.hashtable'
      ]
    }));

    // Swap buffers. This means that the up to date buffer is now the back buffer
    // and we will calculate the front buffer this update.
    particlesNode.getAttributesDGNode().bindings.append(scene.constructOperator({
      operatorName: 'copyCurrentFrameDataToPrevFrameData',
      srcFile: 'FABRIC_ROOT/SG/KL/particles.kl',
      entryPoint: 'copyCurrentFrameDataToPrevFrameData',
      parameterLayout: [
        'self.positions',
        'self.velocities',
        'self.previousframe_positions',
        'self.previousframe_velocities'
      ]
    }));

    // This operator counts all the points which exist in each
    // cell and writes thier ids to the hash table. This operator
    // is single threaded and should be the last to execute.
    particlesNode.getAttributesDGNode().bindings.append(scene.constructOperator({
      operatorName: 'populateHashTable',
      srcFile: 'FABRIC_ROOT/SG/KL/particles.kl',
      entryPoint: 'populateHashTable',
      parameterLayout: [
        'uniforms.hashtable',
        'self.cellindices<>'
      ]
    }));

    return particlesNode;
  }});


});
