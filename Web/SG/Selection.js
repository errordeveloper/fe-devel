/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC.define(["SG/SceneGraph",
               "SG/Geometry"], function() {
  
/**
 * The rselection manager provides selection functionality to the scene graph.
 * additional rendering pipeline and capabilities.
 */
FABRIC.SceneGraph.registerManagerType('SelectionManager', {
  briefDesc: 'The SelectionManager manages selections of objects.',
  detailedDesc: '.',
  parentManagerDesc: '',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      undoManager: undefined
    });

    var selection = [];
    
    var selectionManager = {
      pub:{
        isSelected:function( obj ){ 
          return selection.indexOf(obj) != -1;
        },
        select:function( obj ){
          var prevSelection = selectionManager.pub.getSelectionCopy();
          this.clearSelection(false);
          if(obj.constructor.name == 'Array'){
            for(var i=0; i<obj.length; i++){
              this.addToSelection(obj[i], false);
            }
            selectionManager.pub.fireEvent('selectionChanged', {
              selection: selectionManager.pub.getSelectionCopy(),
              prevSelection: prevSelection
            });
          }
          else{
            this.addToSelection(obj, false);
            selectionManager.pub.fireEvent('selectionChanged', {
              selection: selectionManager.pub.getSelectionCopy(),
              prevSelection: prevSelection
            });
          }
        },
        addToSelection:function(obj, fireevents){
          if(selection.indexOf(obj)!=-1)
            return;
          var prevSelection = selectionManager.pub.getSelectionCopy();
          selection.push(obj);
          if(obj.fireEvent){
            obj.fireEvent('selected');
          }
          if(fireevents!=false){
            selectionManager.pub.fireEvent('selectionChanged', {
              selection: selectionManager.pub.getSelectionCopy(),
              prevSelection: prevSelection
            });
          }
        },
        removeFromSelection:function( obj, firevents ){
          if(selection.length == 0)
            return;
          var id = selection.indexOf(obj);
          if(id != -1){
            var prevSelection = selectionManager.pub.getSelectionCopy();

            // Put the color back. 
            if(obj.fireEvent){
              obj.fireEvent('deselected');
            }
            // Remove from the seleciton array.
            if( selection.length == 1 ){
              selection = []
            }else{
              selection.splice(id,1);
            }
            if(firevents!=false){
              selectionManager.pub.fireEvent('selectionChanged', {
                selection: selectionManager.pub.getSelectionCopy(),
                prevSelection: prevSelection
              });
            }
          }
        },
        clearSelection:function(firevents){ 
          if(selection.length > 0){
            var prevSelection = selectionManager.pub.getSelectionCopy();
            for (var i = selection.length-1; i >=0 ; i--) {
              this.removeFromSelection(selection[i], false);
            }
            if(firevents!=false){
              selectionManager.pub.fireEvent('selectionChanged', {
                selection: selectionManager.pub.getSelectionCopy(),
                prevSelection: prevSelection
              });
            }
          }
        },
        getSelection: function(){
          return selection;
        },
        getSelectionCopy: function(){
          var result = [];
          for(var i=0;i<selection.length;i++)
            result.push(selection[i]);
          return result;
        },
      }
    }
    scene.addEventHandlingFunctions(selectionManager);

    // deal with undo
    var undoManager = options.undoManager;
    selectionManager.pub.getUndoManager = function() {
      return undoManager;
    };
    if(undoManager) {
      selectionManager.pub.addEventListener('selectionChanged', function(evt){
        undoManager.addTransaction({
          name: 'SelectionChanged',
          onRedo: function(){
            selectionManager.pub.select(evt.selection);
          },
          onUndo: function(){
            selectionManager.pub.select(evt.prevSelection);
          }
        });
      });
    }
    
    return selectionManager;
  }});



FABRIC.SceneGraph.registerManagerType('ViewportSelectionManager', {
  briefDesc: 'The ViewportSelectorManager manages selections of objects view mouse events from the viewport.',
  detailedDesc: '.',
  parentManagerDesc: '',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      addToSelectionKeyName: 'ctrlKey',
      removeFromSelectionKeyName: 'altKey',
      enabled: true
    });
    
    var selectionManager = scene.constructManager('SelectionManager', options);
    var enabled = options.enabled;
    selectionManager.pub.setEnabled = function(val) {
      enabled = val;
    };
    
    scene.pub.addEventListener('mouseover_geom', function(evt){
      if(!enabled)
        return;
      if(evt.targetNode.fireEvent){
        evt.targetNode.fireEvent('highlight');
        scene.pub.redrawAllViewports();
      }
    });
    scene.pub.addEventListener('mouseout_geom', function(evt){
      if(!enabled)
        return;
      if(evt.targetNode.fireEvent){
        evt.targetNode.fireEvent('unhighlight');
        scene.pub.redrawAllViewports();
      }
    });
    scene.pub.addEventListener('mousedown_geom', function(evt){
      if(!enabled)
        return;
      if(evt[options.addToSelectionKeyName]){
        selectionManager.pub.addToSelection(evt.targetNode);
      }else if(evt[options.removeFromSelectionKeyName]){
        selectionManager.pub.removeFromSelection(evt.targetNode);
      }else{
        selectionManager.pub.select(evt.targetNode);
        evt.stopPropagation();
        scene.pub.redrawAllViewports();
      }
    });
    var mouseDownScreenPos;
    scene.pub.addEventListener('mousedown', function(evt){
      if(!enabled)
        return;
      if(!evt.targetNode){
        mouseDownScreenPos = evt.mouseScreenPos;
      }
    });
    scene.pub.addEventListener('mouseup', function(evt){
      if(!enabled)
        return;
      if(mouseDownScreenPos && evt.mouseScreenPos.subtract(mouseDownScreenPos).length() < 0.01){
        selectionManager.pub.clearSelection();
        scene.pub.redrawAllViewports();
        mouseDownScreenPos = undefined;
      }
    });
    
    return selectionManager;
  }});


/**
 * The rselection manager provides selection functionality to the scene graph.
 * additional rendering pipeline and capabilities.
 */
FABRIC.SceneGraph.registerManagerType('SelectionManipulationManager', {
  briefDesc: 'The SelectionManipulationManager manages manipulatoin of selected of objects.',
  detailedDesc: '.',
  parentManagerDesc: '',
  optionsDesc: {
  },
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      selectionManager: undefined,
      manipulators: undefined,
      transformGetter: 'getGlobalXfo',
      transformSetter: 'setGlobalXfo',
      screenTranslationRadius: 2.0
      });
    
    var selectionManager = options.selectionManager;
    if(!selectionManager)
      throw("options.selectionManager is not defined!");
    
    // Construct a 'Transform' node that is used as the parent of the Maipulators.
    // This transform node aligns its self with the average of the selected objects.
    // an event is fired on the node when it is manipulated, and it uses this
    // event to manipulate all the selected objects.
    var groupTransform = scene.constructNode('Transform', {
      name: 'SelectionManipulationManagerGroupTransform',
      hierarchical: false
    } );

    var undoManager = scene.getManager('UndoManager');
    
    /////
    // Propagate the manipulation to all selected members.
    var dragStartGlobalXfo, dragStartXfos;
    var dragStartFn = function(evt) {
      dragStartGlobalXfo = groupTransform.pub.getGlobalXfo();
      var selection = selectionManager.getSelection();
      var dragEndXfos = [];
      var dragStartXfosLocal = [];
      dragStartXfos = [];
      for(var i=0; i<selection.length; i++){
        var startXfo = selection[i].getTransformNode()[options.transformGetter]();
        dragStartXfos.push(startXfo);
        dragStartXfosLocal.push(startXfo);
      }
      if(undoManager){
        undoManager.addTransaction({
          name: 'SelectionManipulation',
          onClose: function() {
            var selection = selectionManager.getSelection();
            for(var i=0; i<selection.length; i++){
              dragEndXfos.push(selection[i].getTransformNode()[options.transformGetter]());
            }
          },
          onUndo: function (){
            var selection = selectionManager.getSelection();
            for(var i=0;i<selection.length;i++){
              selection[i].getTransformNode()[options.transformSetter](dragStartXfosLocal[i]);
            }
          },
          onRedo: function (){
            var selection = selectionManager.getSelection();
            for(var i=0;i<selection.length;i++){
              selection[i].getTransformNode()[options.transformSetter](dragEndXfos[i]);
            }
          }
        });
      }
    }
    
    groupTransform.pub.setGlobalXfo = function(xfo){
      if(undoManager && undoManager.undoRedoing()){
        return;
      }
      var xform = xfo.multiply(dragStartGlobalXfo.inverse());
      var selection = selectionManager.getSelection();
      for(var i=0; i<selection.length; i++){
        selection[i].getTransformNode()[options.transformSetter]( xform.multiply(dragStartXfos[i]) );
      }
      selectionManager.fireEvent('selectionManipulated', { selection: selection });
    };
    
    groupTransform.getDGNode().addMember('xfoOffsets','Xfo[]');
    groupTransform.addMemberInterface(groupTransform.getDGNode(), 'xfoOffsets', true);

   // Create a list of known operators for each selection count
   var operatorAssigned = false;
   var generatedOperator;
   var generateTransformOperator = function(){
    
    if(generatedOperator)
      return generatedOperator;
    
     var parameterLayout = [
       'self.globalXfo',
       'self.xfoOffsets',
       'minX.globalXfo',
       'maxX.globalXfo',
       'minY.globalXfo',
       'maxY.globalXfo',
       'minZ.globalXfo',
       'maxZ.globalXfo'
     ]
     
     var srcCodeHeader = 'use Xfo, Mat44;\noperator selMgrCalcAverageXfo( \n  io Xfo xfo';
     srcCodeHeader += ',\n  io Xfo xfoOffsets[]';
     srcCodeHeader += ',\n  io Xfo minX';
     srcCodeHeader += ',\n  io Xfo maxX';
     srcCodeHeader += ',\n  io Xfo minY';
     srcCodeHeader += ',\n  io Xfo maxY';
     srcCodeHeader += ',\n  io Xfo minZ';
     srcCodeHeader += ',\n  io Xfo maxZ\n) {\n';
     var srcCodeBody = '  xfo.setIdentity();\n  Scalar weight = 1.0/6.0;\n';
     srcCodeBody += '  xfo.tr += (minX * xfoOffsets[0]).tr * weight;\n'
     srcCodeBody += '  xfo.tr += (maxX * xfoOffsets[1]).tr * weight;\n'
     srcCodeBody += '  xfo.tr += (minY * xfoOffsets[2]).tr * weight;\n'
     srcCodeBody += '  xfo.tr += (maxY * xfoOffsets[3]).tr * weight;\n'
     srcCodeBody += '  xfo.tr += (minZ * xfoOffsets[4]).tr * weight;\n'
     srcCodeBody += '  xfo.tr += (maxZ * xfoOffsets[5]).tr * weight;\n'
     srcCodeBody += '  xfo.ori = minX.ori;\n'
     srcCodeBody += '  \n}';
     
     var operatorName = 'selMgrCalcAverageXfo';
     generatedOperator = scene.constructOperator( {
         operatorName: operatorName,
         srcCode: (srcCodeHeader + srcCodeBody),
         parameterLayout: parameterLayout,
         entryPoint: 'selMgrCalcAverageXfo',
         async: false
       });
     
     return generatedOperator;
   }

    // retrieve the list of manipulators    
    var manipulators = [];
    var manipulatorsDrawToggle = false;
    var toggleManipulatorsDisplay = function(drawToggle){
      if(!manipulators)
        return;
      manipulatorsDrawToggle = drawToggle;
      for(var i=0; i<manipulators.length; i++){
        manipulators[i].setDrawToggle(drawToggle);
      }
    }

    var prevSelectionCount = 0;
    selectionManager.addEventListener('selectionChanged', function(evt){
      if(operatorAssigned){
        groupTransform.getDGNode().bindings.remove(0);
        operatorAssigned = false;
      }
      if(evt.selection.length==0){
        toggleManipulatorsDisplay(false);
      }
      else{
        toggleManipulatorsDisplay(true);
        
        var minX = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        var maxX = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        var minY = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        var maxY = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        var minZ = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        var maxZ = scene.getPrivateInterface(evt.selection[0].getTransformNode());
        for(var i=1;i<evt.selection.length;i++) {
          var selectionXfo = evt.selection[i].getTransformNode()[options.transformGetter]();
          if(selectionXfo.tr.x < minX.pub[options.transformGetter]().tr.x)
            minX = scene.getPrivateInterface(evt.selection[i].getTransformNode());
          if(selectionXfo.tr.x > maxX.pub[options.transformGetter]().tr.x)
            maxX = scene.getPrivateInterface(evt.selection[i].getTransformNode());
          if(selectionXfo.tr.y < minY.pub[options.transformGetter]().tr.y)
            minY = scene.getPrivateInterface(evt.selection[i].getTransformNode());
          if(selectionXfo.tr.y > maxY.pub[options.transformGetter]().tr.y)
            maxY = scene.getPrivateInterface(evt.selection[i].getTransformNode());
          if(selectionXfo.tr.z < minZ.pub[options.transformGetter]().tr.z)
            minZ = scene.getPrivateInterface(evt.selection[i].getTransformNode());
          if(selectionXfo.tr.z > maxZ.pub[options.transformGetter]().tr.z)
            maxZ = scene.getPrivateInterface(evt.selection[i].getTransformNode());
        }
        
        // compute the offsets
        var offsets = [];
        var centroid = new FABRIC.RT.Xfo();
        centroid.ori = minX.pub[options.transformGetter]().ori;
        centroid.tr.x = (minX.pub[options.transformGetter]().tr.x + maxX.pub[options.transformGetter]().tr.x) * 0.5;
        centroid.tr.y = (minY.pub[options.transformGetter]().tr.y + maxY.pub[options.transformGetter]().tr.y) * 0.5;
        centroid.tr.z = (minZ.pub[options.transformGetter]().tr.z + maxZ.pub[options.transformGetter]().tr.z) * 0.5;
        offsets.push(minX.pub[options.transformGetter]().inverse().multiply(centroid));
        offsets.push(maxX.pub[options.transformGetter]().inverse().multiply(centroid));
        offsets.push(minY.pub[options.transformGetter]().inverse().multiply(centroid));
        offsets.push(maxY.pub[options.transformGetter]().inverse().multiply(centroid));
        offsets.push(minZ.pub[options.transformGetter]().inverse().multiply(centroid));
        offsets.push(maxZ.pub[options.transformGetter]().inverse().multiply(centroid));
        
        groupTransform.pub.setXfoOffsets(offsets);
        
        groupTransform.getDGNode().setDependency(minX.getDGNode(), 'minX');
        groupTransform.getDGNode().setDependency(maxX.getDGNode(), 'maxX');
        groupTransform.getDGNode().setDependency(minY.getDGNode(), 'minY');
        groupTransform.getDGNode().setDependency(maxY.getDGNode(), 'maxY');
        groupTransform.getDGNode().setDependency(minZ.getDGNode(), 'minZ');
        groupTransform.getDGNode().setDependency(maxZ.getDGNode(), 'maxZ');
        
        groupTransform.getDGNode().bindings.append(generateTransformOperator());
        operatorAssigned = true;

        prevSelectionCount = evt.selection.length;
      }
    });

    
    // setup the public interface
    var selectionManipulationManager = {
      pub:{
        getTransformNode : function() {
          return groupTransform.pub;
        },
        addManipulator: function(manipulator) {
          manipulator.addEventListener('dragstart', dragStartFn);
        //  manipulator.addEventListener('drag', dragFn);
        //  manipulator.addEventListener('dragend', dragEndFn);
          manipulator.setDrawToggle(manipulatorsDrawToggle);
          manipulators.push(manipulator);
        },
        toggleManipulatorsDisplay: function(value) {
          toggleManipulatorsDisplay(value);
        },
        getManipulatorsDrawToggle: function() {
          return manipulatorsDrawToggle;
        },
        getSelectionManager: function(){
          return selectionManager;
        }
      }
    };
    toggleManipulatorsDisplay(false);
    
    if(!options.manipulators){
      selectionManipulationManager.pub.addManipulator( scene.pub.constructNode('ScreenTranslationManipulator', {
          name: options.name + 'ScreenTranslationManipulator',
          color: FABRIC.RT.rgb(0, 0.8, 0, 1),
          parentNode: groupTransform.pub,
          geometryNode: scene.pub.constructNode('Sphere', {radius: options.screenTranslationRadius, detail: 8.0}),
          drawToggle: false,
          drawOverlaid: true
        }));
    } else {
      for(var i=0; i < options.manipulators.length; i++) {
        selectionManipulationManager.pub.addManipulator( options.manipulators[i] );
      }
    }
    
    return selectionManipulationManager;
  }});

FABRIC.SceneGraph.registerNodeType('SelectableInstance', {
  factoryFn: function(options, scene) {
    scene.assignDefaults(options, {
      highlightMaterial: undefined,
      selectMaterial: undefined,
      removeMainMaterial: false
    });
    
    var selectableInstance = scene.constructNode('Instance', options);
    var mainMaterialNode = options.materialNode;
    options.removeMainMaterial = mainMaterialNode ? options.removeMainMaterial : false;
    
    var selectable = true;
    selectableInstance.pub.isSelectable = function(){
      return selectable;
    }
    selectableInstance.pub.setSelectable = function(val){
      selectable = val;
    }
    var highlighted = false;
    var selected = false;
    var removedMainMaterial = undefined;
    selectableInstance.pub.addEventListener('highlight', function(evt) {
      if(!selected && selectable){
        if(options.removeMainMaterial) {
          selectableInstance.pub.removeMaterialNode( mainMaterialNode );
          removedMainMaterial = true;
        }
        selectableInstance.pub.addMaterialNode( options.highlightMaterial );
        highlighted = true;
      }
    });
    selectableInstance.pub.addEventListener('unhighlight', function(evt) {
      if(highlighted){
        selectableInstance.pub.removeMaterialNode( options.highlightMaterial );
        if(removedMainMaterial) {
          selectableInstance.pub.addMaterialNode( mainMaterialNode );
          removedMainMaterial = undefined;
        }
        highlighted = false;
      }
    });
    selectableInstance.pub.addEventListener('selected', function(evt) {
      if(!selectable)
        return;
      if(highlighted){
        selectableInstance.pub.removeMaterialNode( options.highlightMaterial );
        highlighted = false;
      } else if(options.removeMainMaterial) {
        selectableInstance.pub.removeMaterialNode( mainMaterialNode );
        removedMainMaterial = true;
      }
      selectableInstance.pub.addMaterialNode( options.selectMaterial );
      selected = true;
    });
    selectableInstance.pub.addEventListener('deselected', function(evt) {
      selectableInstance.pub.removeMaterialNode( options.selectMaterial );
      if(removedMainMaterial) {
        selectableInstance.pub.addMaterialNode( mainMaterialNode );
        removedMainMaterial = undefined;
      }
      selected = false;
    });
    return selectableInstance;
  }});


});
