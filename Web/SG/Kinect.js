/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Geometry",
               "SG/Images"], function() {
  
FABRIC.RT.KinectCamera = function(options) {
  if(!options)
    options = {};
  this.localData = null;
  this.initiated = false;
  this.tiltAngle = 0;
  this.width = 320;
  this.height = 240;
  this.supportsColor = options.supportsColor == undefined ? true : options.supportsColor;
  this.supportsDepth = options.supportsDepth == undefined ? false : options.supportsDepth;
  this.supportsSkeleton = options.supportsSkeleton == undefined ? false : options.supportsSkeleton;
  this.colorData = [];
  this.playerData = [];
  this.depthData = [];
  this.skeletonData = [];
  this.supportsSmoothing = options.supportsSmoothing == undefined ? true : options.supportsSmoothing;;
  this.skeletonSmoothing = options.skeletonSmoothing == undefined ? 0.05 : options.skeletonSmoothing;;
  this.skeletonCorrection = options.skeletonCorrection == undefined ? 0.5 : options.skeletonCorrection;;
  this.skeletonPrediction = options.skeletonPrediction == undefined ? 0.5 : options.skeletonPrediction;;
  this.skeletonJitterRadius = options.skeletonJitterRadius == undefined ? 0.05 : options.skeletonJitterRadius;;
  this.skeletonMaxDeviationRadius = options.skeletonMaxDeviationRadius == undefined ? 0.04 : options.skeletonMaxDeviationRadius;;
};

FABRIC.SceneGraph.registerNodeType('KinectCamera', {
  briefDesc: 'The KinectCamera represents a Microsoft Kinect Camera.',
  detailedDesc: 'The KinectCamera represents a Microsoft Kinect Camera, providing access to its rgba and depth channel, as well as NUI skeleton data.',
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      supportsColor: false,
      supportsDepth: false,
      supportsSkeleton: false,
      tiltAngle: 0,
      depthMin: 800,
      depthMax: 1500
    });
    
    // create the bullet node
    var kinectNode = scene.constructNode('SceneGraphNode', {name: 'KinectCameraNode'});
    var dgnode = kinectNode.constructDGNode('DGNode');
    dgnode.addMember('camera', 'KinectCamera', new FABRIC.RT.KinectCamera(options));
    dgnode.addMember('tiltAngle', 'Integer', options.tiltAngle);
    dgnode.addMember('depthMin', 'Integer', options.depthMin);
    dgnode.addMember('depthMax', 'Integer', options.depthMax);
    kinectNode.addMemberInterface(dgnode, 'camera', false);
    kinectNode.addMemberInterface(dgnode, 'tiltAngle', true);
    kinectNode.addMemberInterface(dgnode, 'depthMin', true);
    kinectNode.addMemberInterface(dgnode, 'depthMax', true);
    dgnode.setDependency(scene.getGlobalsNode(), 'globals');

    // create init operator
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'initKinectCamera',
      parameterLayout: [
        'self.camera'
      ],
      entryPoint: 'initKinectCamera',
      srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
    }));

    // create tilt operator
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'tiltKinectCamera',
      parameterLayout: [
        'self.camera',
        'self.tiltAngle'
      ],
      entryPoint: 'tiltKinectCamera',
      srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
    }));

    // create tilt operator
    dgnode.bindings.append(scene.constructOperator({
      operatorName: 'getKinectCameraAllPixels',
      parameterLayout: [
        'self.camera',
        'globals.time'
      ],
      entryPoint: 'getKinectCameraAllPixels',
      srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
    }));
    
    // see if we need to support color
    if(options.supportsColor) {
      
      kinectNode.pub.constructColorTextureNode = function() {
        var textureNode = scene.constructNode('Image2D', {
          format: 'RGBA',
          createResourceLoadNode: false,
          createDgNodes: true,
          width: 640,
          height: 480,
          forceRefresh: true,
          initImage: false
        });
        var texturePixelsDGNode = textureNode.getPixelsDGNode();
        texturePixelsDGNode.setDependency(dgnode,'kinect');

        // create init operators
        texturePixelsDGNode.bindings.append(scene.constructOperator({
          operatorName: 'getKinectCameraColorPixels',
          parameterLayout: [
            'self',
            'kinect.camera',
            'uniforms.width',
            'uniforms.height',
            'self.pixels<>'
          ],
          entryPoint: 'getKinectCameraColorPixels',
          srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
        }));
        
        return textureNode.pub;
      };
    }
    
    // see if we need to support depth
    if(options.supportsDepth) {
      
      kinectNode.pub.constructDepthTextureNode = function() {
        var textureNode = scene.constructNode('Image2D', {
          format: 'RGBA',
          createResourceLoadNode: false,
          createDgNodes: true,
          width: 320,
          height: 240,
          forceRefresh: true,
          initImage: false
        });
        
        var texturePixelsDGNode = textureNode.getPixelsDGNode();
        texturePixelsDGNode.setDependency(dgnode,'kinect');

        // create init operators
        texturePixelsDGNode.bindings.append(scene.constructOperator({
          operatorName: 'getKinectCameraDepthAsColorPixels',
          parameterLayout: [
            'self',
            'kinect.camera',
            'self.pixels<>',
            'uniforms.width',
            'uniforms.height',
            'kinect.depthMin',
            'kinect.depthMax'
          ],
          entryPoint: 'getKinectCameraDepthAsColorPixels',
          srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
        }));

        return textureNode.pub;
      };
    }

    // see if we need to support skeletons
    if(options.supportsSkeleton) {
      
      kinectNode.pub.constructLinesNode = function() {
        var linesNode = scene.constructNode('Lines', {});
        linesNode.pub.addVertexAttributeValue('vertexColors','Color',{ genVBO:true });
        linesNode.pub.setAttributeDynamic('positions');
        linesNode.pub.setAttributeDynamic('vertexColors');
        var linesAttributesDGNode = linesNode.getAttributesDGNode();
        linesAttributesDGNode.setDependency(scene.getGlobalsNode(), 'globals');
        linesAttributesDGNode.setDependency(dgnode,'kinect');
        linesAttributesDGNode.resize(120); // 6 skeletons, 20 positions
        var indices = [];
        for(var i=0;i<6;i++){
          indices.push(i*20+0);  // KINECT_POSITION_HIP_CENTER
          indices.push(i*20+1);  // KINECT_POSITION_SPINE
          indices.push(i*20+1);  // KINECT_POSITION_SPINE
          indices.push(i*20+2);  // KINECT_POSITION_SHOULDER_CENTER
          indices.push(i*20+2);  // KINECT_POSITION_SHOULDER_CENTER
          indices.push(i*20+3);  // KINECT_POSITION_HEAD
          indices.push(i*20+2);  // KINECT_POSITION_SHOULDER_CENTER
          indices.push(i*20+4);  // KINECT_POSITION_SHOULDER_LEFT
          indices.push(i*20+4);  // KINECT_POSITION_SHOULDER_LEFT
          indices.push(i*20+5);  // KINECT_POSITION_ELBOW_LEFT
          indices.push(i*20+5);  // KINECT_POSITION_ELBOW_LEFT
          indices.push(i*20+6);  // KINECT_POSITION_WRIST_LEFT
          indices.push(i*20+6);  // KINECT_POSITION_WRIST_LEFT
          indices.push(i*20+7);  // KINECT_POSITION_HAND_LEFT
          indices.push(i*20+2);  // KINECT_POSITION_SHOULDER_CENTER
          indices.push(i*20+8);  // KINECT_POSITION_SHOULDER_RIGHT
          indices.push(i*20+8);  // KINECT_POSITION_SHOULDER_RIGHT
          indices.push(i*20+9);  // KINECT_POSITION_ELBOW_RIGHT
          indices.push(i*20+9);  // KINECT_POSITION_ELBOW_RIGHT
          indices.push(i*20+10); // KINECT_POSITION_WRIST_RIGHT
          indices.push(i*20+10); // KINECT_POSITION_WRIST_RIGHT
          indices.push(i*20+11); // KINECT_POSITION_HAND_RIGHT
          indices.push(i*20+0);  // KINECT_POSITION_HIP_CENTER
          indices.push(i*20+12);  // KINECT_POSITION_HIP_LEFT
          indices.push(i*20+12);  // KINECT_POSITION_HIP_LEFT
          indices.push(i*20+13);  // KINECT_POSITION_KNEE_LEFT
          indices.push(i*20+13);  // KINECT_POSITION_KNEE_LEFT
          indices.push(i*20+14);  // KINECT_POSITION_ANKLE_LEFT
          indices.push(i*20+14);  // KINECT_POSITION_ANKLE_LEFT
          indices.push(i*20+15);  // KINECT_POSITION_FOOT_LEFT
          indices.push(i*20+0);  // KINECT_POSITION_HIP_CENTER
          indices.push(i*20+16);  // KINECT_POSITION_HIP_RIGHT
          indices.push(i*20+16);  // KINECT_POSITION_HIP_RIGHT
          indices.push(i*20+17);  // KINECT_POSITION_KNEE_RIGHT
          indices.push(i*20+17);  // KINECT_POSITION_KNEE_RIGHT
          indices.push(i*20+18);  // KINECT_POSITION_ANKLE_RIGHT
          indices.push(i*20+18);  // KINECT_POSITION_ANKLE_RIGHT
          indices.push(i*20+19);  // KINECT_POSITION_FOOT_RIGHT
        }
        linesNode.getUniformsDGNode().setData('indices',0,indices);
  
        // create init operator
        linesAttributesDGNode.bindings.append(scene.constructOperator({
          operatorName: 'getKinectCameraLines',
          parameterLayout: [
            'kinect.camera',
            'self.positions<>',
            'self.vertexColors<>'
          ],
          entryPoint: 'getKinectCameraLines',
          srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
        }));
        return linesNode.pub;
      };
    }

    kinectNode.pub.constructPointsNode = function() {
      var pointsNode = scene.constructNode('Points', {});
      pointsNode.pub.addVertexAttributeValue('vertexColors','Color',{ genVBO:true });
      pointsNode.pub.setAttributeDynamic('positions');
      pointsNode.pub.setAttributeDynamic('vertexColors');
      var pointsAttributesDGNode = pointsNode.getAttributesDGNode();
      pointsAttributesDGNode.setDependency(scene.getGlobalsNode(), 'globals');
      pointsAttributesDGNode.setDependency(dgnode,'kinect');
      pointsAttributesDGNode.resize(320*240);

      // create init operator
      pointsAttributesDGNode.bindings.append(scene.constructOperator({
        operatorName: 'getKinectCameraPoints',
        parameterLayout: [
          'kinect.camera',
          'self.positions<>',
          'self.vertexColors<>'
        ],
        entryPoint: 'getKinectCameraPoints',
        srcFile: 'FABRIC_ROOT/SG/KL/kinect.kl'
      }));

      return pointsNode.pub;
    };
    
    return kinectNode;
  }});

});
