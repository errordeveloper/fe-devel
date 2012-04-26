/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph", "RT/KeyframeTrack", "RT/LinearKeyframe", "RT/BezierKeyframe"], function() {
  

FABRIC.SceneGraph.registerNodeType('AnimationContainer', {
  briefDesc: 'The AnimationContainer node implements an array of animation tracks.',
  detailedDesc: 'The AnimationContainer node is an abstract node type that stores an animation track per-slice. Drived nodes must specify the KeyframeType that is being interpollated',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    keyframetype: 'Specified by the derrived node, this option specifies the type of the keyframe tract that will be interpollated.',
    name:'The name of the track. This value is displayed in the curve Editor'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        keyframetype: undefined,
        controllerNode: undefined
      });
    
    if (options.keyframetype == undefined) {
      throw ('Please specify a type of data to interpollate');
    }
    var keyframeType = options.keyframetype;
    var keyframeTrackType = options.keyframetype+'Track';
    var rTypes = scene.getRegisteredTypesManager().getRegisteredTypes();
    if (!rTypes[options.keyframetype]) {
      throw ('Type "' + options.keyframetype + '" is not registered. Load the RT file.');
    }
    if (!rTypes[keyframeTrackType]) {
      throw ('Type "' + keyframeTrackType + '" is not registered. Load the RT file.');
    }
    var defaultKeyframeValue = rTypes[options.keyframetype].defaultValue;
    if (!defaultKeyframeValue.valueType) {
      throw ('The keyframe registered type must provide a ' +
          'valueType that indicates what the keyframes will evaluate to. ' +
          'Normally this is a Scalar, but could be a Vec2, Color or any other value');
    }
    
    var animationNode = scene.constructNode('SceneGraphNode', options);
    
    animationNode.pub.getKeyframeType = function() {
      return keyframeType;
    };
    animationNode.getTrackType = function() {
      return keyframeTrackType;
    };
    
    animationNode.getControllerDGNode = function() {
      if(options.controllerNode)
        return scene.getPrivateInterface(options.controllerNode).getDGNode();
      return scene.getGlobalsNode();
    };

    animationNode.getControllerBinding = function() {
      if(options.controllerNode)
        return 'controller.localTime';
      return 'controller.time';
    };
    
    animationNode.pub.getValueType = function(trackid) {
      // To determing what kind of data this evaluator should evaluate to,
      // we look up the keyframe type in the type manager, and request the
      // default value, which when is created on demand, enables us to call the valueType
      return defaultKeyframeValue.valueType;
    };
     
    animationNode.getEvaluateCurveOperator = function() {
      throw "Derrived nodes must impliment this function.";
    };
    return animationNode;
  }
});



FABRIC.SceneGraph.registerNodeType('TrackAnimationContainer', {
  briefDesc: 'The TrackAnimationContainer node implements an array of animation tracks.',
  detailedDesc: 'The TrackAnimationContainer node is an abstract node type that stores an animation track per-slice. Drived nodes must specify the KeyframeType that is being interpollated',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    keyframetype: 'Specified by the derrived node, this option specifies the type of the keyframe tract that will be interpollated.',
    name:'The name of the track. This value is displayed in the curve Editor'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        keyframetype: undefined
      });
    
    var trackAnimationContainerNode = scene.constructNode('AnimationContainer', options);
    var dgnode = trackAnimationContainerNode.constructDGNode('DGNode');
    dgnode.addMember('track', trackAnimationContainerNode.getTrackType());
    
    var firstTrackAdded = false;
    trackAnimationContainerNode.pub.addTrack = function(track) {
      var trackId;
      if(!firstTrackAdded){
        // Nodes default to having 1 slice, so we use up the first slot here.
        trackId = 0;
        firstTrackAdded = true;
      }
      else{
        trackId = dgnode.size();
        dgnode.resize(trackId+1);
      }
      dgnode.setData('track', trackId, track);
      return trackId;
    };
    trackAnimationContainerNode.pub.getTimeRange = function() {
      var range = new FABRIC.RT.Vec2();
      var numTracks = dgnode.size();
      for(var i=0; i<numTracks; i++){
        var track = dgnode.getData('track', i);
        if(track.keys.length<=1){
          continue;
        }
        if(i==0){
          range.set(track.keys[0].time, track.keys[track.keys.length-1].time);
        }else{
          if(track.keys[0].time < range.x) range.x = track.keys[0].time;
          if(track.keys[track.keys.length-1].time > range.y) range.y = track.keys[track.keys.length-1].time;
        }
      }
      return range;
    };
    
    trackAnimationContainerNode.pub.getTrack = function(trackId) {
      return dgnode.getData('track', trackId);
    };
    trackAnimationContainerNode.pub.setTrack = function(track, trackId) {
      dgnode.setData('track', trackId ? trackId : 0, track);
    };
    trackAnimationContainerNode.pub.getTracks = function() {
      return dgnode.getMemberBulkData('track');
    };
    trackAnimationContainerNode.pub.setTracks = function(tracks) {
      return dgnode.setMemberBulkData(trackSet, trackSetId);
    };
    
    trackAnimationContainerNode.pub.getTrackCount = function() {
      return dgnode.size();
    };
    
    // Because we store all tracks in a track set, getting and setting the
    // tracks causes significant performance issues. So the solution for
    // now is to disable track evaluation while manipulating, and only store
    // the tracks once manipulaiton is complete. 
    var m_track, m_trackId, m_newUndoTransaction;
    trackAnimationContainerNode.pub.setValues = function(time, trackIds, values) {
    //  var trackSet = this.getTrackSet(trackSetId);
    //  trackSet.setValues(time, trackIds, values);
    //  this.setTrackSet(trackSet, trackSetId);
    //  trackAnimationContainerNode.pub.fireEvent('keyframetrackchanged', {});
    
      m_track.setValues(time, trackIds, values);
    };
    trackAnimationContainerNode.beginManipulation = function(trackId) {
      m_trackId = trackId;
      m_track = trackAnimationContainerNode.pub.getTrack(trackId);
      
      var undoManager = scene.getManager('UndoManager');
      if(undoManager){
        m_newUndoTransaction = false;
        if(!undoManager.undoTransactionOpen()){
          undoManager.openUndoTransaction();
          m_newUndoTransaction = true;
        }
        var newTrackSet, prevTrackSet = trackAnimationContainerNode.pub.getTrack(m_trackId);
        undoManager.addTransaction({
          onClose: function() {
            newTrackSet = m_track;//trackAnimationContainerNode.pub.getTrack(m_trackId);
          },
          onUndo: function() {
            trackAnimationContainerNode.pub.setTrack(prevTrackSet, m_trackId);
          },
          onRedo: function() {
            trackAnimationContainerNode.pub.setTrack(newTrackSet, m_trackId);
          }
        });
      }
    }
    trackAnimationContainerNode.endManipulation = function() {
      trackAnimationContainerNode.pub.setTrack(m_track, m_trackId);
      
      var undoManager = scene.getManager('UndoManager');
      if(undoManager && m_newUndoTransaction){
        undoManager.closeUndoTransaction();
      }
      trackAnimationContainerNode.pub.fireEvent('keyframetrackchanged', {});
    }
        
    trackAnimationContainerNode.getEvaluateCurveOperator = function() {
      return scene.constructOperator({
          operatorName: 'evaluate'+trackAnimationContainerNode.pub.getKeyframeType()+'Curve',
          srcFile: 'FABRIC_ROOT/SG/KL/evaluateKeyframeAnimationTrack.kl',
          preProcessorDefinitions: {
            KEYFRAMETYPE: trackAnimationContainerNode.pub.getKeyframeType(),
            KEYFRAME_EVALUATEDTYPE: trackAnimationContainerNode.pub.getValueType()
          },
          entryPoint: 'evaluateTrackCurve',
          parameterLayout: [
            'characteranimationcontainer.track<>',
            'parameters.timeRange',
            'parameters.segmentCount',
            'self.values',
            'self.index',
          ],
          async: false
        });
    };
    
    trackAnimationContainerNode.pub.bindNodeMembersToTracks = function(targetnode, memberBindings, trackSetId, targetName) {
      // Generate a unique operator that binds this 
      // animation curves to the target node's members.
      trackSetId = trackSetId ? trackSetId : 0;

      targetnode = scene.getPrivateInterface(targetnode);
      targetnode.getDGNode().setDependency(dgnode, 'trackAnimationContainers');
      targetnode.getDGNode().setDependency(trackAnimationContainerNode.getControllerDGNode(), 'controller');
      var rTypes = scene.getRegisteredTypesManager().getRegisteredTypes();

      var targetNodeMembers = targetnode.getDGNode().getMembers();
      var trackDataType = trackAnimationContainerNode.pub.getValueType();
      var operatorName;
      if(targetName){
        operatorName = 'bindKeyframeTracksTo' + targetName;
      }
      else{
        operatorName = 'bindKeyframeTracksTo' + JSON.stringify(memberBindings).replace(/[^a-zA-Z 0-9]+/g, '');
      }
      var operatorBodySrc = '';
      var operatorHeaderSrc = '\nuse Vec3, Euler, Quat, RotationOrder;\n';
      operatorHeaderSrc += 'use '+trackAnimationContainerNode.pub.getKeyframeType() +';\n';
      operatorHeaderSrc += 'use '+trackAnimationContainerNode.getTrackType()+';\n';
      operatorHeaderSrc += '\n\noperator ' + operatorName + '(\n';
      operatorHeaderSrc += '  io Scalar time,\n';
      operatorHeaderSrc += '  io ' + trackAnimationContainerNode.getTrackType() + ' tracks<>';
      var operatorArraySrc = {};
      var parameterLayout = [trackAnimationContainerNode.getControllerBinding(), 'trackAnimationContainers.track<>'];
      var numEvaluatedTracks = 0;
      var addTrackEvaluation = function(trackid){
        numEvaluatedTracks++;
        return 'tracks['+trackid+'].evaluate(time, currKeys['+ (numEvaluatedTracks-1) +'])';
      }
      var tempVariables = {};
      for (var memberAccessor in memberBindings) {
        var memberBinding = memberBindings[memberAccessor];
        var splitMemberAccessor = memberAccessor.split('.');
        var memberName = splitMemberAccessor[0];
        var memberIndex = undefined;
        if (memberName.indexOf('[') > 0) {
          memberIndex = memberName.substr(memberName.indexOf('[') + 1);
          memberIndex = parseInt(memberIndex.substr(0, memberIndex.indexOf(']')));
          memberName = memberName.substr(0, memberName.indexOf('['));
          if (operatorArraySrc[memberName]) {
            if (operatorArraySrc[memberName] < memberIndex) {
              operatorArraySrc[memberName] = memberIndex;
            }
          }
          else {
            operatorArraySrc[memberName] = memberIndex;
          }
        }
        var boundMemberType = targetNodeMembers[memberName].type;
        var isArray = false;
        if (boundMemberType.indexOf('[') > 0) {
          boundMemberType = boundMemberType.substr(0, boundMemberType.indexOf('['));
          isArray = true;
        }
        var memberType = boundMemberType;
        var typeDesc = rTypes[memberType];
        for (i = 1; i < splitMemberAccessor.length; i++) {
          var newMemberType = null;
          for (var j = 0; j < typeDesc.members.length; ++j) {
            if (typeDesc.members[j].name == splitMemberAccessor[i]) {
              newMemberType = typeDesc.members[j].type;
              break;
            }
          }
          if (!newMemberType)
            throw 'unable to find member "' + splitMemberAccessor[i] + '" of type "' + memberType + '"';
          memberType = newMemberType;
          typeDesc = rTypes[memberType];
        }
        var memberBindingCode = '';
        switch (memberType) {
          case 'Integer':

            break;
          case 'Scalar':
            memberBindingCode = memberAccessor + ' = ' + addTrackEvaluation(memberBinding);
            break;
          case 'Vec3':
            if (memberBinding[0] >= 0)
              memberBindingCode += '  ' + memberAccessor + '.x = ' + addTrackEvaluation(memberBinding[0]) + ';\n';
            if (memberBinding[1] >= 0)
              memberBindingCode += '  ' + memberAccessor + '.y = ' + addTrackEvaluation(memberBinding[1]) + ';\n';
            if (memberBinding[2] >= 0)
              memberBindingCode += '  ' + memberAccessor + '.z = ' + addTrackEvaluation(memberBinding[2]) + ';\n';
            break;
          case 'Quat':
            if (memberBinding.length === 3) {
              if (!tempVariables['tempVec3']) {
                tempVariables['tempVec3'] = '  var Vec3 tempVec3;\n';
              }
              if (!tempVariables['tempEuler']) {
                tempVariables['tempEuler'] = '  var Euler tempEuler;\n';
              }
              if (memberBinding[0] >= 0)
                memberBindingCode += '  tempVec3.x = ' + addTrackEvaluation(memberBinding[0]) + ';\n';
              else
                memberBindingCode += '  tempVec3.x = 0.0;\n';
              if (memberBinding[1] >= 0)
                memberBindingCode += '  tempVec3.y = ' + addTrackEvaluation(memberBinding[1]) + ';\n';
              else
                memberBindingCode += '  tempVec3.y = 0.0;\n';
              if (memberBinding[2] >= 0)
                memberBindingCode += '  tempVec3.z = ' + addTrackEvaluation(memberBinding[2]) + ';\n';
              else
                memberBindingCode += '  tempVec3.z = 0.0;\n';
              memberBindingCode += '  tempEuler = Euler(tempVec3);\n';
              memberBindingCode += '  ' + memberAccessor + '.setFromEuler(tempEuler);\n';
            }else if (memberBinding.length === 4) {
              if (memberBinding[0] < 0 || memberBinding[1] < 0 || memberBinding[2] < 0  || memberBinding[3] < 0 ){
                throw 'incorrect binding for Quat:' + JSON.stringify(memberBinding);
              }
              memberBindingCode += '  ' + memberAccessor + '.set( '+ addTrackEvaluation(memberBinding[0]) + ',\n';
              memberBindingCode += '    ' + addTrackEvaluation(memberBinding[1]) + ',\n';
              memberBindingCode += '    ' + addTrackEvaluation(memberBinding[2]) + ',\n';
              memberBindingCode += '    ' + addTrackEvaluation(memberBinding[3]) + ');\n';
              memberBindingCode += '  ' + memberAccessor + ' = ' + memberAccessor + '.unit();\n';
            }
            break;

        }
        
        operatorBodySrc += '\n' + memberBindingCode;
        
        if (parameterLayout.indexOf('self.' + memberName) == -1) {
          operatorHeaderSrc += ',\n  io ' + boundMemberType + ' ' + memberName + (isArray ? '[]' : '');
          parameterLayout.push('self.' + memberName);
        }
      }

      // add the array resize code to ensure that they are big enough
      operatorHeaderSrc += '\n){';
      for (var arrayName in operatorArraySrc) {
        operatorHeaderSrc += '\n  if(' + arrayName + '.size() < ' + (operatorArraySrc[arrayName] + 1) + '){';
        operatorHeaderSrc += '\n    ' + arrayName + '.resize(' + (operatorArraySrc[arrayName] + 1) + ');';
        operatorHeaderSrc += '\n  }';
      }

      // add the temporary varibale declaration at the top of the function body.
      for (i in tempVariables) {
        operatorBodySrc = tempVariables[i] + operatorBodySrc;
      }
      operatorBodySrc = '  Integer currKeys['+numEvaluatedTracks+'];\n' + operatorBodySrc;
      operatorBodySrc = '\n' + operatorBodySrc + '}';

      if (targetnode.getDGNode().bindings.length > 0) {
        targetnode.getDGNode().bindings.insert(scene.constructOperator({
          operatorName: operatorName,
          srcCode: operatorHeaderSrc + operatorBodySrc,
          entryPoint: operatorName,
          parameterLayout: parameterLayout
        }), 0);
      }
      else {
        targetnode.getDGNode().bindings.append(scene.constructOperator({
          operatorName: operatorName,
          srcCode: operatorHeaderSrc + operatorBodySrc,
          entryPoint: operatorName,
          parameterLayout: parameterLayout
        }));
      }
    };
    
    trackAnimationContainerNode.pub.openCurveEditor = function(options) {
      options = options ? options : {};
      var curveEditorWindow = window.open(
        FABRIC.processURL('FABRIC_ROOT/SG/CurveEditor/CurveEditor.html') + '?id=' + scene.pub.getContextId(),
        'Fabric Curve Editor:' + (options.name ? options.name : trackAnimationContainerNode.pub.getName()),
        'status=1,resizable=1,width='+window.innerWidth+',height='+(window.innerHeight * 0.6)
      );
      curveEditorWindow.characterAnimationContainerNode = trackAnimationContainerNode.pub;
      curveEditorWindow.trackSetId = options.trackSetId ? options.trackSetId : 0;
      curveEditorWindow.drawKeys = options.drawKeys ? options.drawKeys : true;
      curveEditorWindow.trackFilters = options.trackFilters ? options.trackFilters : [];
      curveEditorWindow.scene = scene.pub;
      return curveEditorWindow;
    }
    
    var parentWriteData = trackAnimationContainerNode.writeData;
    var parentReadData = trackAnimationContainerNode.readData;
    trackAnimationContainerNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      constructionOptions.keyframetype = options.keyframetype;
      nodeData.numTracks = dgnode.size();
      nodeData.tracks = dgnode.getBulkData('track');
    };
    trackAnimationContainerNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      dgnode.resize(nodeData.numTracks);
      for(var i=0; i<nodeData.numTracks; i++){
        dgnode.setData('track', i, nodeData.tracks.track[i]);
      }
    };
    
    scene.addEventHandlingFunctions(trackAnimationContainerNode);
    return trackAnimationContainerNode;
  }});



FABRIC.SceneGraph.registerNodeType('CharacterAnimationContainer', {
  briefDesc: 'The CharacterAnimationContainer node implements an array of animation tracks.',
  detailedDesc: 'The CharacterAnimationContainer node is an abstract node type that stores an animation track per-slice. Drived nodes must specify the KeyframeType that is being interpollated',
  parentNodeDesc: 'AnimationContainer',
  optionsDesc: {
    keyframetype: 'Specified by the derrived node, this option specifies the type of the keyframe tract that will be interpollated.',
    name:'The name of the track. This value is displayed in the curve Editor'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        keyframetype: undefined
      });

    
    var characterAnimationContainerNode = scene.constructNode('AnimationContainer', options);
    var dgnode = characterAnimationContainerNode.constructDGNode('DGNode');
    
    var keyframeTrackSetType = characterAnimationContainerNode.pub.getKeyframeType()+'TrackSet';
    var keyframeTrackSetBindingsType = characterAnimationContainerNode.pub.getKeyframeType()+'TrackSetBindings';
    
    dgnode.addMember('trackSet', keyframeTrackSetType);
    dgnode.addMember('bindings', keyframeTrackSetBindingsType);
    
    var firstTrackAdded = false;
    characterAnimationContainerNode.pub.addTrackSet = function(trackset, bindings) {
      var trackSetId;
      if(!firstTrackAdded){
        // Nodes default to having 1 slice, so we use up the first slot here.
        trackSetId = 0;
        firstTrackAdded = true;
      }
      else{
        trackSetId = dgnode.size();
        dgnode.resize(trackSetId+1);
      }
      dgnode.setData('trackSet', trackSetId, trackset);
      if(bindings){
        dgnode.setData('bindings', trackSetId, bindings);
      }
      characterAnimationContainerNode.pub.fireEvent('tracksetadded', { trackSetId: trackSetId } );
      return trackSetId;
    };
    characterAnimationContainerNode.pub.getTimeRange = function(trackSetId) {
      var calcTimeRange = function(trackSet){
        var range = new FABRIC.RT.Vec2();
        for(var i=0; i<trackSet.tracks.length; i++){
          var track = trackSet.tracks[i];
          if(track.keys.length<=1){
            continue;
          }
          if(i==0){
            range.set(track.keys[0].time, track.keys[track.keys.length-1].time);
          }else{
            if(track.keys[0].time < range.x) range.x = track.keys[0].time;
            if(track.keys[track.keys.length-1].time > range.y) range.y = track.keys[track.keys.length-1].time;
          }
        }
        return range;
      }
      return calcTimeRange(dgnode.getData('trackSet', trackSetId ? trackSetId : 0));
    };

    characterAnimationContainerNode.getTrackSetType = function() {
      return keyframeTrackSetType;
    };

    characterAnimationContainerNode.newTrackSet = function(trackSetName) {
      return new FABRIC.RT[keyframeTrackSetType](trackSetName);
    };
    
    characterAnimationContainerNode.pub.getTrackSetCount = function() {
      return dgnode.size();
    };
    characterAnimationContainerNode.pub.getTrackSet = function(trackSetId) {
      return dgnode.getData('trackSet', trackSetId);
    };
    characterAnimationContainerNode.pub.setTrackSet = function(trackSet, trackSetId) {
      return dgnode.getData('trackSet', trackSetId, trackSet);
    };
    
    characterAnimationContainerNode.getBindings = function(trackSetId) {
      return dgnode.getData('bindings', trackSetId);
    };
    
    characterAnimationContainerNode.pub.getTrackSetName = function(trackSetId) {
      return dgnode.getData('trackSet', trackSetId).name;
    };
    
    characterAnimationContainerNode.pub.getTracks = function(trackSetId) {
      return dgnode.getData('trackSet', trackSetId).tracks;
    };
    characterAnimationContainerNode.pub.setTracks = function(tracks, trackSetId) {
      var trackSet = dgnode.getData('trackSet', trackSetId);
      trackSet.tracks = tracks;
      dgnode.setData('trackSet', trackSetId, trackSet);
    };
    characterAnimationContainerNode.pub.setTrack = function(track, trackId, trackSetId) {
      var trackSet = dgnode.getData('trackSet', trackSetId);
      trackSet.tracks[trackId] = track;
      dgnode.setData('trackSet', trackSetId, trackSet);
    };
    
    // Because we store all tracks in a track set, getting and setting the
    // tracks causes significant performance issues. So the solution for
    // now is to disable track evaluation while manipulating, and only store
    // the tracks once manipulaiton is complete. 
    var m_trackSet, m_trackSetId, m_newUndoTransaction;
    characterAnimationContainerNode.pub.setValues = function(trackSetId, time, value, index, xfoTrackFilter) {
      
      var type = (typeof value == 'number') ? 'Number' : value.getType();
      var bindings = characterAnimationContainerNode.getBindings(trackSetId);
      var findBinding = function(bindingsList){
        for(var i=0; i<bindingsList.length; i++){
          if(bindingsList[i].varId == index){
            return bindingsList[i];
          }
        }
        throw "Binding not found";
      }
      var binding, values;
      var trackIds;
      switch(type){
      case 'Number':
        binding = findBinding(bindings.scalarBindings);
        values = [value];
        trackIds = binding.trackIds;
        break;
      case 'FABRIC.RT.Vec3':
        binding = findBinding(bindings.vec3Bindings);
        values = [value.x, value.y, value.z];
        trackIds = binding.trackIds;
        break;
      case 'FABRIC.RT.Quat':
        binding = findBinding(bindings.quatBindings);
        if(binding.trackIds.length == 3){
          var euler = new FABRIC.RT.Euler();
          euler.setFromQuat(value);
          values = [euler.x, euler.y, euler.z];
        }else if(binding.trackIds.length == 4){
          values = [value.w, value.x, value.y, value.z];
        }
        trackIds = binding.trackIds;
        break;
      case 'FABRIC.RT.Xfo':
        binding = findBinding(bindings.xfoBindings);
        trackIds = binding.trackIds;
        if(binding.trackIds.length == 6){
          var euler = new FABRIC.RT.Euler();
          euler.setFromQuat(value.ori);
          values = [value.tr.x, value.tr.y, value.tr.z, euler.x, euler.y, euler.z];
        }else if(binding.trackIds.length == 7){
          
          if(xfoTrackFilter){
            var filteredValues = [];
            var filteredTrackIds = [];
            if(xfoTrackFilter.tr){
              filteredValues = [value.tr.x, value.tr.y, value.tr.z];
              filteredTrackIds = [trackIds[0], trackIds[1], trackIds[2]]
            }
            if(xfoTrackFilter.ori){
              filteredValues = filteredValues.concat([value.ori.v.x, value.ori.v.y, value.ori.v.z, value.ori.w]);
              filteredTrackIds = filteredTrackIds.concat([trackIds[3], trackIds[4], trackIds[5], trackIds[6]]);
            }
            if(xfoTrackFilter.sc){
              filteredValues = [value.sc.x, value.sc.y, value.sc.z];
              filteredTrackIds = [trackIds[6], trackIds[7], trackIds[8]]
            }
            values = filteredValues;
            trackIds = filteredTrackIds;
          }
          else{
            values = [value.tr.x, value.tr.y, value.tr.z, value.ori.v.x, value.ori.v.y, value.ori.v.z, value.ori.w];
            trackIds = binding.trackIds;
          }
        }
        break;
      default:
        throw 'Unhandled type:' + val;
      }
      
      // This is commented out, because we fetch the
      // track set at the beginning of the manipulation
      // and push it only at the end.
    //  var trackSet = this.getTrackSet(trackSetId);
    //  trackSet.setValues(time, trackIds, values);
    //  this.setTrackSet(trackSet, trackSetId);
    //  characterAnimationContainerNode.pub.fireEvent('keyframetrackchanged', {});
    
      m_trackSet.setValues(time, trackIds, values);
    };
    characterAnimationContainerNode.beginManipulation = function(trackSetId) {
      m_trackSetId = trackSetId;
      m_trackSet = dgnode.getData('trackSet', trackSetId);
      
      var undoManager = scene.getManager('UndoManager');
      if(undoManager){
        // A new transaction may have already been opened if the 
        // event has already been recieved from a different source. 
        if(!m_newUndoTransaction){
          if(!undoManager.undoTransactionOpen()){
            undoManager.openUndoTransaction();
            m_newUndoTransaction = true;
          }
          var newTrackSet, prevTrackSet = characterAnimationContainerNode.pub.getTrackSet(m_trackSetId);
          undoManager.addTransaction({
            onClose: function() {
              newTrackSet = m_trackSet;//characterAnimationContainerNode.pub.getTrackSet(m_trackId);
            },
            onUndo: function() {
              m_trackSet = prevTrackSet;
              dgnode.setData('trackSet', m_trackSetId, prevTrackSet);
              characterAnimationContainerNode.pub.fireEvent('keyframetrackchanged', { trackSetId: m_trackSetId });
            },
            onRedo: function() {
              m_trackSet = newTrackSet;
              dgnode.setData('trackSet', m_trackSetId, newTrackSet);
              characterAnimationContainerNode.pub.fireEvent('keyframetrackchanged', { trackSetId: m_trackSetId });
            }
          });
        }
      }
    }
    characterAnimationContainerNode.endManipulation = function() {
      dgnode.setData('trackSet', m_trackSetId, m_trackSet);
      
      var undoManager = scene.getManager('UndoManager');
      if(undoManager && m_newUndoTransaction){
        undoManager.closeUndoTransaction();
        m_newUndoTransaction = false;
      }
      characterAnimationContainerNode.pub.fireEvent('keyframetrackchanged', {});
    }
    
    var paramsdgnode;
    characterAnimationContainerNode.pub.bindToRig = function(rigNode, trackSetId){
      if (!rigNode.isTypeOf('CharacterRig')) {
        throw ('Incorrect type. Must be a CharacterRig');
      }
      
      rigNode = scene.getPrivateInterface(rigNode);
        
      if(!paramsdgnode){
        paramsdgnode = characterAnimationContainerNode.constructDGNode('ParamsDGNode');
        
        dgnode.setDependency(paramsdgnode, 'params');
        paramsdgnode.addMember('boundTrack', 'Integer', trackSetId);
      }else{
        paramsdgnode.setData('boundTrack', 0, trackSetId);
      }
    //  dgnode.setData('bindings', trackSetId, trackBindings);
      
      characterAnimationContainerNode.pub.plotKeyframes = function(trackSetId, timeRange, sampleFrequency){
        var variablesNode = scene.getPrivateInterface(rigNode.pub.getVariablesNode());
        dgnode.setDependency(variablesNode.getDGNode(), 'variables');
        dgnode.setDependency(characterAnimationContainerNode.getControllerDGNode(), 'controller');
        
        paramsdgnode.setData('boundTrack', 0, trackSetId);
        dgnode.bindings.append(scene.constructOperator({
          operatorName: 'keyCurvesFromVariables',
          srcFile: 'FABRIC_ROOT/SG/KL/keyCurvesFromVariables.kl',
          preProcessorDefinitions: {
            KEYFRAMETRACKSETTYPE: keyframeTrackSetType
          },
          entryPoint: 'keyCurvesFromVariables',
          parameterLayout: [
            characterAnimationContainerNode.getControllerBinding(),
            'variables.poseVariables',
            'params.boundTrack',
            'self.trackSet<>',
            'self.bindings<>'
          ],
          async: false
        }));
        var t = 0;
        for(var i=0; i <= Math.round((timeRange.y-timeRange.x) / sampleFrequency); i++){
          scene.pub.animation.setTime(t, false);
          dgnode.evaluate();
          t += sampleFrequency;
        }
        dgnode.bindings.remove(0);
        dgnode.removeDependency('variables');
        dgnode.removeDependency('controller');
        scene.pub.animation.setTime(0, false);
      }
    //  return trackBindings;
    }
        
    characterAnimationContainerNode.getEvaluateCurveOperator = function() {
      return scene.constructOperator({
          operatorName: 'evaluate'+options.keyframetype+'Curve',
          srcFile: 'FABRIC_ROOT/SG/KL/evaluateKeyframeAnimationTrackSet.kl',
          preProcessorDefinitions: {
            KEYFRAMETYPE: characterAnimationContainerNode.pub.getKeyframeType(),
            TRACKTYPE: characterAnimationContainerNode.getTrackType(),
            TRACKSETTYPE: characterAnimationContainerNode.getTrackSetType(),
            KEYFRAME_EVALUATEDTYPE: characterAnimationContainerNode.pub.getValueType()
          },
          entryPoint: 'evaluateTrackSetCurve',
          parameterLayout: [
            'characteranimationcontainer.trackSet<>',
            'parameters.trackSetId',
            'parameters.timeRange',
            'parameters.segmentCount',
            'self.values',
            'self.index',
          ],
          async: false
        });
    };
    
    characterAnimationContainerNode.pub.openCurveEditor = function(options) {
      options = options ? options : {};
      var curveEditorWindow = window.open(
        FABRIC.processURL('FABRIC_ROOT/SG/CurveEditor/CurveEditor.html') + '?id=' + scene.pub.getContextId(),
        'Fabric Curve Editor:' + (options.name ? options.name : characterAnimationContainerNode.pub.getName()),
        'status=1,resizable=1,width='+window.innerWidth+',height='+(window.innerHeight * 0.6)
      );
      curveEditorWindow.characterAnimationContainerNode = characterAnimationContainerNode.pub;
      curveEditorWindow.trackSetId = options.trackSetId!=undefined ? options.trackSetId : 0;
      curveEditorWindow.drawKeys = options.drawKeys!=undefined ? options.drawKeys : true;
      curveEditorWindow.trackFilters = options.trackFilters!=undefined ? options.trackFilters : [];
      curveEditorWindow.scene = scene.pub;
      return curveEditorWindow;
    }
    
    
    
    var parentWriteData = characterAnimationContainerNode.writeData;
    var parentReadData = characterAnimationContainerNode.readData;
    characterAnimationContainerNode.writeData = function(sceneSerializer, constructionOptions, nodeData) {
      parentWriteData(sceneSerializer, constructionOptions, nodeData);
      constructionOptions.keyframetype = options.keyframetype;
      nodeData.numTracks = characterAnimationContainerNode.pub.getTrackSetCount();
      nodeData.trackSets = [];
      nodeData.bindings = [];
      for(var i=0; i<nodeData.numTracks; i++){
        nodeData.trackSets.push(dgnode.getData('trackSet', i));
        nodeData.bindings.push(dgnode.getData('bindings', i));
      }
    };
    characterAnimationContainerNode.readData = function(sceneDeserializer, nodeData) {
      parentReadData(sceneDeserializer, nodeData);
      dgnode.resize(nodeData.numTracks);
      for(var i=0; i<nodeData.numTracks; i++){
        dgnode.setData('trackSet', i, nodeData.trackSets[i]);
        dgnode.setData('bindings', i, nodeData.bindings[i]);
      }
    };
    
    scene.addEventHandlingFunctions(characterAnimationContainerNode);
    return characterAnimationContainerNode;
  }});


FABRIC.SceneGraph.registerNodeType('LinearTrackAnimationContainer', {
  briefDesc: 'The LinearTrackAnimationContainer node implements an array of linear keyframe animation tracks.',
  detailedDesc: 'The LinearTrackAnimationContainer node derrives from TrackAnimationContainer and specifies that the tracks should contains \'LinearKeyframes\'',
  parentNodeDesc: 'TrackAnimationContainer',
  factoryFn: function(options, scene) {
    options.keyframetype = 'LinearKeyframe';
    return scene.constructNode('TrackAnimationContainer', options);
  }});

FABRIC.SceneGraph.registerNodeType('LinearCharacterAnimationContainer', {
  briefDesc: 'The LinearCharacterAnimationContainer node implements an array of linear keyframe animation tracks.',
  detailedDesc: 'The LinearCharacterAnimationContainer node derrives from TrackAnimationContainer and specifies that the tracks should contains \'LinearKeyframes\'',
  parentNodeDesc: 'CharacterAnimationContainer',
  factoryFn: function(options, scene) {
    options.keyframetype = 'LinearKeyframe';
    return scene.constructNode('CharacterAnimationContainer', options);
  }});


FABRIC.SceneGraph.registerNodeType('BezierTrackAnimationContainer', {
  briefDesc: 'The BezierTrackAnimationContainer node implements an array of bezier keyframe animation tracks.',
  detailedDesc: 'The BezierTrackAnimationContainer node derrives from TrackAnimationContainer and specifies that the tracks should contains \'BezierKeyframe\'',
  parentNodeDesc: 'TrackAnimationContainer',
  factoryFn: function(options, scene) {
    options.keyframetype = 'BezierKeyframe';
    return scene.constructNode('TrackAnimationContainer', options);
  }});

FABRIC.SceneGraph.registerNodeType('BezierCharacterAnimationContainer', {
  briefDesc: 'The BezierCharacterAnimationContainer node implements an array of bezier keyframe animation tracks.',
  detailedDesc: 'The BezierCharacterAnimationContainer node derrives from TrackAnimationContainer and specifies that the tracks should contains \'BezierKeyframe\'',
  parentNodeDesc: 'CharacterAnimationContainer',
  factoryFn: function(options, scene) {
    options.keyframetype = 'BezierKeyframe';
    return scene.constructNode('CharacterAnimationContainer', options);
  }});


FABRIC.SceneGraph.registerNodeType('ColorTrackAnimationContainer', {
  briefDesc: 'The ColorTrackAnimationContainer node implements an array of color keyframe animation tracks.',
  detailedDesc: 'The ColorTrackAnimationContainer node derrives from TrackAnimationContainer and specifies that the tracks should contains \'ColorKeyframe\'',
  parentNodeDesc: 'TrackAnimationContainer',
  factoryFn: function(options, scene) {
    options.keyframetype = 'ColorKeyframe';
    return scene.constructNode('TrackAnimationContainer', options);
  }});


FABRIC.SceneGraph.registerNodeType('AnimationController', {
  briefDesc: 'The AnimationController node controls the time at which an TrackAnimationContainer is evaluated.',
  detailedDesc: 'The Animation Controller only maintains time values which can be used by'+
                'animation evaluators. Many evaluators can share an Animation Controller',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
    playbackRate: 'The rate to increment the time values, relative to the global scene time increments.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        playbackRate: 1.0,
        bindToGlobalTime: true,
        timeRange: new FABRIC.RT.Vec2(0, 10),
        timeControl: 0, /* 0: absolute, 1:increment */
        outOfRange: 1 /* 0: linear, 1:loop, 3:clamp */
      });

    var animationControllerNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = animationControllerNode.constructDGNode('DGNode');
    dgnode.addMember('timeControl', 'Integer', options.timeControl);
    dgnode.addMember('playbackRate', 'Scalar', options.playbackRate);
    dgnode.addMember('localTime', 'Scalar', options.timeRange.x);
    dgnode.addMember('timeRange', 'Vec2', options.timeRange);
    dgnode.addMember('outOfRange', 'Integer', options.outOfRange);
    
    // create a getter and setter for the local time
    animationControllerNode.addMemberInterface(dgnode, 'playbackRate', true);
    animationControllerNode.addMemberInterface(dgnode, 'localTime', true);
    animationControllerNode.addMemberInterface(dgnode, 'timeRange', true);

    // extend public interface
    animationControllerNode.pub.getTime = function(time) {
      return dgnode.getData('localTime');
    };
    animationControllerNode.pub.setTime = function(time) {
      dgnode.setData('localTime', 0, time);
    };

    // Here, the animation controllers time is locked to
    // global time. Often the time context for an animation
    // is computed, or simulated based on scene events.
    if(options.bindToGlobalTime) {
      dgnode.setDependency(scene.getGlobalsNode(), 'globals');
      
      // This operator might simulate the platback local time by incrementing
      // it each frame by a rate specified by the playbackRate parameter.
      dgnode.bindings.append(scene.constructOperator( {
          operatorName:'incrementControllerLocalTime',
          srcFile:'FABRIC_ROOT/SG/KL/incrementControllerLocalTime.kl',
          entryPoint:'incrementControllerLocalTime',
          parameterLayout: [
            'globals.time',
            'globals.timestep',
            'self.timeControl',
            'self.playbackRate',
            'self.timeRange',
            'self.outOfRange',
            'self.localTime'
          ]
        }));
    }
    return animationControllerNode;
  }});



FABRIC.SceneGraph.registerNodeType('TrackDisplay', {
  briefDesc: 'The TrackDisplay evaluates TrackAnimationContainers across time ranges ready for display in the CurveEditor.',
  detailedDesc: 'The TrackDisplay evaluates TrackAnimationContainers across time ranges ready for display in the CurveEditor.',
  parentNodeDesc: 'SceneGraphNode',
  optionsDesc: {
      characterAnimationContainerNode: 'The CharacterAnimationContainer node to evaluate.',
      trackIndex: 'The index of the track in the TrackAnimationContainer node to evaluate.',
      timeRange: 'The time rage to evaluate.',
      segmentCount: 'The number of steps to divide the timeRange by.'
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
        trackAnimationContainerNode: undefined,
        characterAnimationContainerNode: undefined,
        segmentCount: 100,
        timeRange: new FABRIC.RT.Vec2(0, 1),
        trackSetId: 0
      });

    var trackDisplayNode = scene.constructNode('SceneGraphNode', options);
    var dgnode = trackDisplayNode.constructDGNode('DGNode');
    var parametersdgnode = trackDisplayNode.constructDGNode('ParametersDGNode');
    var characterAnimationContainerNode;

    parametersdgnode.addMember('trackSetId', 'Integer', options.trackSetId);
    parametersdgnode.addMember('timeRange', 'Vec2', options.timeRange);
    parametersdgnode.addMember('segmentCount', 'Size', options.segmentCount);

    dgnode.setDependency(parametersdgnode, 'parameters');
    
    trackDisplayNode.addMemberInterface(parametersdgnode, 'trackSetId', true);
    trackDisplayNode.addMemberInterface(parametersdgnode, 'timeRange', true);
    trackDisplayNode.addMemberInterface(parametersdgnode, 'segmentCount', true);
    
    var tracks;
    trackDisplayNode.addReferenceInterface('CharacterAnimationContainer', 'AnimationContainer',
      function(nodePrivate, trackSetId){
        characterAnimationContainerNode = nodePrivate;
        var trackDataType = characterAnimationContainerNode.pub.getValueType();
        tracks = characterAnimationContainerNode.pub.getTracks(trackSetId);
        dgnode.setDependency(characterAnimationContainerNode.getDGNode(), 'characteranimationcontainer');
  
        dgnode.addMember('values', trackDataType+'[]');
        parametersdgnode.setData('trackSetId', trackSetId ? trackSetId : 0);
        dgnode.resize(tracks.length);
  
        dgnode.bindings.append(characterAnimationContainerNode.getEvaluateCurveOperator());
      });
    trackDisplayNode.pub.getCurveData = function(trackIndex) {
      if(tracks && tracks.length > 0){
        dgnode.evaluate();
        return dgnode.getData('values', trackIndex);
      }
    }

    if (options.trackAnimationContainerNode) {
      trackDisplayNode.pub.setCharacterAnimationContainerNode(options.trackAnimationContainerNode);
    }
    if (options.characterAnimationContainerNode) {
      trackDisplayNode.pub.setCharacterAnimationContainerNode(options.characterAnimationContainerNode, options.trackSetId);
    }
    return trackDisplayNode;
  }});

});
