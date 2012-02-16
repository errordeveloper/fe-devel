(function(){return (
function (fabricClient, logCallback, debugLogCallback) {

  var queuedCommands = [];
  var queuedUnwinds = [];
  var queuedCallbacks = [];

  var executeQueuedCommands = function() {
    if (queuedCommands.length > 0) {
      var commands = queuedCommands;
      queuedCommands = [];
      var callbacks = queuedCallbacks;
      queuedCallbacks = [];
      var unwinds = queuedUnwinds;
      queuedUnwinds = [];

      var jsonEncodedCommands = JSON.stringify(commands);
      if (debugLogCallback)
        debugLogCallback('-> ' + jsonEncodedCommands);

      var jsonEncodedResults = fabricClient.jsonExec(jsonEncodedCommands);
      if (debugLogCallback)
        debugLogCallback('<- ' + jsonEncodedResults);

      var results;
      try {
        results = JSON.parse(jsonEncodedResults);
      }
      catch (e) {
        throw 'unable to parse JSON results:' + jsonEncodedResults;
      }
      for (var i = 0; i < results.length; ++i) {
        var result = results[i];
        var callback = callbacks[i];
        if ('exception' in result) {
          for (var j = unwinds.length; j-- > i; ) {
            var unwind = unwinds[j];
            if (unwind)
              unwind();
          }
          throw 'Fabric core exception: ' + result.exception;
        }
        else if (callback)
          callback(result.result);
      }
    }
  };

  var queueCommand = function(dst, cmd, arg, unwind, callback) {
    var command = {
      dst: dst,
      cmd: cmd
    };
    if (arg !== undefined)
      command.arg = arg;

    queuedCommands.push(command);
    queuedUnwinds.push(unwind);
    queuedCallbacks.push(callback);

    if (queuedCommands.length >= 262144)
      executeQueuedCommands();
  };

  var createRT = function() {
    var RT = {};

    RT.assignPrototypes = function(data, typeName) {
      if (typeName.substring(typeName.length - 2) == '[]') {
        typeName = typeName.substring(0, typeName.length - 2);
        for (var i = 0; i < data.length; ++i) {
          RT.assignPrototypes(data[i], typeName);
        }
      }
      else if (typeName in RT.prototypes) {
        data.__proto__ = RT.prototypes[typeName];
        if ('members' in RT.registeredTypes[typeName]) {
          var members = RT.registeredTypes[typeName].members;
          for (var i = 0; i < members.length; ++i) {
            var member = members[i];
            RT.assignPrototypes(data[member.name], member.type);
          }
        }
      }
      return data;
    };

    RT.queueCommand = function(cmd, arg, unwind, callback) {
      queueCommand(['RT'], cmd, arg, unwind, callback);
    };

    RT.patch = function(diff) {
      if ('registeredTypes' in diff) {
        RT.registeredTypes = {};
        for (var typeName in diff.registeredTypes) {
          RT.registeredTypes[typeName] = diff.registeredTypes[typeName];
        }
      }
    };

    RT.handleStateNotification = function(state) {
      RT.prototypes = {};
      RT.patch(state);
    };

    RT.handle = function(cmd, arg) {
      switch (cmd) {
        case 'delta':
          this.patch(arg);
          break;
        default:
          throw "command '" + cmd + "': unrecognized";
      }
    };

    RT.route = function(src, cmd, arg) {
      if (src.length == 0) {
        handle(cmd, arg);
      }
      else if (src.length == 1) {
        var typeName = src[0];
        try {
          switch (cmd) {
            case 'delta':
              RT.registeredTypes[typeName] = arg;
              RT.registeredTypes[typeName].defaultValue =
                RT.assignPrototypes(RT.registeredTypes[typeName].defaultValue, typeName);
              break;
            default:
              throw 'unrecognized';
          }
        }
        catch (e) {
          throw "'" + cmd + "': " + e;
        }
      }
      else
        throw 'unroutable';
    };

    RT.pub = {
      getRegisteredTypes: function() {
        executeQueuedCommands();
        return RT.registeredTypes;
      },

      registerType: function(name, desc) {
        var members = [];
        for (var descMemberName in desc.members) {
          var member = {
            name: descMemberName,
            type: desc.members[descMemberName]
          };
          members.push(member);
        }

        var defaultValue = new desc.constructor();
        RT.prototypes[name] = defaultValue.__proto__;

        var arg = {
          name: name,
          members: members,
          defaultValue: defaultValue
        };
        if ('klBindings' in desc)
          arg.klBindings = desc.klBindings;

        RT.queueCommand('registerType', arg, function() {
          delete RT.prototypes[name];
        });
      }
    };

    return RT;
  };
  var RT = createRT();

  var createDG = function() {
    var DG = {
      namedObjects: {}
    };

    DG.queueCommand = function(dst, cmd, arg, unwind, callback) {
      queueCommand(['DG'].concat(dst), cmd, arg, unwind, callback);
    };

    DG.createBinding = function() {
      var result = {
        parameterLayout: []
      };

      result.pub = {};

      result.pub.getOperator = function() {
        return result.operator;
      };

      result.pub.setOperator = function(operator) {
        result.operator = operator;
      };

      result.pub.getParameterLayout = function() {
        return result.parameterLayout;
      };

      result.pub.setParameterLayout = function(parameterLayout) {
        result.parameterLayout = parameterLayout;
      };

      return result;
    };

    DG.createBindingList = function(dst) {
      var result = { bindings: [] };

      result.patch = function(state) {
        result.bindings = [];
        for (var i = 0; i < state.length; ++i) {
          var binding = {
            operator: DG.namedObjects[state[i].operator].pub,
            parameterLayout: state[i].parameterLayout
          };
          result.bindings.push(binding);
        }
      };

      result.handle = function(cmd, arg) {
        switch (cmd) {
          case 'delta':
            result.patch(arg);
            break;
          default:
            throw "command '" + cmd + "': unrecognized";
        }
      };

      result.route = function(src, cmd, arg) {
        if (src.length == 0) {
          result.handle(cmd, arg);
        }
        else
          throw 'unroutable';
      };

      result.handleStateNotification = function(state) {
        result.patch(state);
      };

      result.pub = {};

      result.pub.empty = function() {
        if (!('bindings' in result))
          executeQueuedCommands();
        return result.bindings.length == 0;
      };

      result.pub.getLength = function() {
        if (!('bindings' in result))
          executeQueuedCommands();
        return result.bindings.length;
      };

      result.pub.getOperator = function(index) {
        if (!('bindings' in result))
          executeQueuedCommands();
        return result.bindings[index].operator;
      };

      result.pub.append = function(binding) {
        var operatorName;
        try {
          operatorName = binding.getOperator().getName();
        }
        catch (e) {
          throw 'operator: not an operator';
        }

        var oldBindings = result.bindings;
        delete result.bindings;

        DG.queueCommand(dst, 'append', {
          operatorName: operatorName,
          parameterLayout: binding.getParameterLayout()
        }, function() {
          result.bindings = oldBindings;
        });
      };

      result.pub.insert = function(binding, beforeIndex) {
        var operatorName;
        try {
          operatorName = binding.getOperator().getName();
        }
        catch (e) {
          throw 'operator: not an operator';
        }

        if (typeof beforeIndex !== 'number')
          throw 'beforeIndex: must be an integer';

        var oldBindings = result.bindings;
        delete result.bindings;

        DG.queueCommand(dst, 'insert', {
          beforeIndex: beforeIndex,
          operatorName: operatorName,
          parameterLayout: binding.getParameterLayout()
        }, function() {
          result.bindings = oldBindings;
        });
      };
      
      result.pub.remove = function(index) {
        var oldBindings = result.bindings;
        delete result.bindings;
        DG.queueCommand(dst, 'remove', {
          index: index,
        }, function() {
          result.bindings = oldBindings;
        });
      };

      return result;
    };

    DG.createNamedObject = function(name) {
      if (name in DG.namedObjects)
        throw "a NamedObject named '" + name + "' already exists";

      var result = {};

      result.name = name;

      result.queueCommand = function(cmd, arg, unwind, callback) {
        if (!this.name)
          throw "NamedObject '" + name + "' has been deleted";
        DG.queueCommand([this.name], cmd, arg, unwind, callback);
      };

      result.patch = function(diff) {
        if ('errors' in diff)
          result.errors = diff.errors;
      };

      result.destroy = function() {
        delete DG.namedObjects[name];
        delete result.name;
      };

      result.handle = function(cmd, arg) {
        switch (cmd) {
          case 'delta':
            result.patch(arg);
            break;
          default:
            throw "command '" + cmd + "': unrecognized";
        }
      };

      result.route = function(src, cmd, arg) {
        if (src.length == 0) {
          result.handle(cmd, arg);
        }
        else
          throw 'unroutable';
      };

      result.pub = {};

      result.pub.getName = function() {
        return result.name;
      };

      result.pub.getErrors = function() {
        executeQueuedCommands();
        return result.errors;
      };

      DG.namedObjects[name] = result;

      return result;
    };

    DG.createOperator = function(name) {
      var result = DG.createNamedObject(name);

      result.diagnostics = [];

      var parentPatch = result.patch;
      result.patch = function(diff) {
        parentPatch(diff);

        if ('filename' in diff)
          result.filename = diff.filename;

        if ('sourceCode' in diff)
          result.sourceCode = diff.sourceCode;

        if ('entryFunctionName' in diff)
          result.entryFunctionName = diff.entryFunctionName;

        if ('diagnostics' in diff)
          result.diagnostics = diff.diagnostics;

        if ('mainThreadOnly' in diff)
          result.mainThreadOnly = diff.mainThreadOnly;
      };

      result.pub.getMainThreadOnly = function() {
        if (!('mainThreadOnly' in result))
          executeQueuedCommands();
        return result.mainThreadOnly;
      }

      result.pub.setMainThreadOnly = function(mainThreadOnly) {
        var oldMainThreadOnly = result.mainThreadOnly;
        result.mainThreadOnly = mainThreadOnly;
        result.queueCommand('setMainThreadOnly', mainThreadOnly, function() {
          result.mainThreadOnly = oldMainThreadOnly;
        });
      };

      result.pub.getFilename = function() {
        if (!('filename' in result))
          executeQueuedCommands();
        return result.filename;
      };

      result.pub.getSourceCode = function() {
        if (!('sourceCode' in result))
          executeQueuedCommands();
        return result.sourceCode;
      };

      var setSourceCode = function(filename, sourceCode) {
        var oldFilename = result.filename;
        result.filename = filename;
        var oldSourceCode = result.sourceCode;
        result.sourceCode = sourceCode;
        var oldDiagnostics = result.diagnostics;
        delete result.diagnostics;
        result.queueCommand('setSourceCode', {
          filename: filename,
          sourceCode: sourceCode
        }, function() {
          result.filename = oldFilename;
          result.sourceCode = oldSourceCode;
          result.diagnostics = oldDiagnostics;
        });
      };
      result.pub.setSourceCode = function(filename, sourceCode) {
        if (!sourceCode) {
          // [pzion 20110919] Legacy usage: assume default filename
          sourceCode = filename;
          filename = "(unknown)";
        }
        setSourceCode(filename, sourceCode);
      };

      result.pub.getEntryFunctionName = function() {
        if (!('entryFunctionName' in result))
          executeQueuedCommands();
        return result.entryFunctionName;
      };

      result.pub.setEntryFunctionName = function(entryFunctionName) {
        var oldEntryFunctionName = result.entryFunctionName;
        result.entryFunctionName = entryFunctionName;
        result.queueCommand('setEntryFunctionName', entryFunctionName, function() {
          result.entryFunctionName = oldEntryFunctionName;
        });
        delete result.diagnostics;
      };

      result.pub.getDiagnostics = function() {
        if (!('diagnostics' in result))
          executeQueuedCommands();
        return result.diagnostics;
      };

      return result;
    };

    DG.createContainer = function(name) {
      var result = DG.createNamedObject(name);

      var parentPatch = result.patch;
      result.patch = function(diff) {
        parentPatch(diff);

        if ('members' in diff)
          result.members = diff.members;

        if ('size' in diff)
          result.size = diff.size;
      };

      var parentHandleNotification = result.handle;
      result.handle = function(cmd, arg) {
        if (cmd == 'dataChange') {
          var memberName = arg.memberName;
          var sliceIndex = arg.sliceIndex;
          /* [pzion 20110524] would invalidate cache here */
        }
        else {
          parentHandleNotification(cmd, arg);
        }
      };

      result.pub.getCount = function() {
        if (!('size' in result))
          executeQueuedCommands();
        return result.size;
      };

      result.pub.size = function() {
        if (!('size' in result))
          executeQueuedCommands();
        return result.size;
      };

      result.pub.setCount = function(count) {
        result.queueCommand('resize', count);
        delete result.size;
      };

      result.pub.resize = function(count) {
        result.queueCommand('resize', count);
        delete result.size;
      };

      result.pub.getMembers = function() {
        if (!('members' in result))
          executeQueuedCommands();
        return result.members;
      };

      result.pub.addMember = function(memberName, memberType, defaultValue) {
        if (!('members' in result))
          result.members = {};
        if (memberName in result.members)
          throw "there is already a member named '" + memberName + "'";

        var arg = {
          'name': memberName,
          'type': memberType
        };
        if (defaultValue !== undefined)
          arg.defaultValue = defaultValue;

        result.members[memberName] = arg;

        result.queueCommand('addMember', arg, function() {
          delete result.members[memberName];
        });
      };

      result.pub.removeMember = function(memberName) {
        if (!('members' in result) || !(memberName in result.members))
          throw "there is no member named '" + memberName + "'";
        var oldMember = result.members[memberName];
        delete result.members[memberName];

        result.queueCommand('removeMember', memberName, function() {
          result.members[memberName] = oldMember;
        });
      };

      result.pub.getData = function(memberName, sliceIndex) {
        if (sliceIndex === undefined)
          sliceIndex = 0;

        var functionResult;
        result.queueCommand('getData', {
          'memberName': memberName,
          'sliceIndex': sliceIndex
        }, function() {
        }, function(data) {
          functionResult = RT.assignPrototypes(data, result.members[memberName].type);
        });
        executeQueuedCommands();
        return functionResult;
      };

      result.pub.getDataJSON = function(memberName, sliceIndex) {
        if (sliceIndex === undefined)
          sliceIndex = 0;

        var functionResult;
        result.queueCommand('getDataJSON', {
          'memberName': memberName,
          'sliceIndex': sliceIndex
        }, function() {
        }, function(data) {
          functionResult = data;
        });
        executeQueuedCommands();
        return functionResult;
      };

      result.pub.getDataSize = function(memberName, sliceIndex) {
        var dataSize;
        result.queueCommand('getDataSize', {
          'memberName': memberName,
          'sliceIndex': sliceIndex
        }, function() {
        }, function(data) {
          dataSize = data;
        });
        executeQueuedCommands();
        return dataSize;
      };

      result.pub.getDataElement = function(memberName, sliceIndex, elementIndex) {
        var dataElement;
        result.queueCommand('getDataElement', {
          'memberName': memberName,
          'sliceIndex': sliceIndex,
          'elementIndex': elementIndex
        }, function() {
        }, function(data) {
          // pull off the last [] since we're looking at an element
          var type = result.members[memberName].type
          dataElement = RT.assignPrototypes(data, type.substring(0, type.length - 2));
        });
        executeQueuedCommands();
        return dataElement;
      };

      result.pub.setData = function(memberName, sliceIndex, data) {
        if (data == undefined) {
          data = sliceIndex;
          sliceIndex = 0;
        }

        result.queueCommand('setData', {
          'memberName': memberName,
          'sliceIndex': sliceIndex,
          'data': data
        });
      };

      result.pub.getBulkData = function() {
        var bulkData;
        result.queueCommand('getBulkData', null, function() { }, function(data) {
          for (var memberName in data) {
            var member = data[memberName];
            for (var i = 0; i < member.length; ++i)
              RT.assignPrototypes(member[i], result.members[memberName].type);
          }
          bulkData = data;
        });
        executeQueuedCommands();
        return bulkData;
      };

      result.pub.setBulkData = function(data) {
        result.queueCommand('setBulkData', data);
      };

      result.pub.getSliceBulkData = function(index) {
        if (typeof index !== 'number') {
          throw 'index: must be an integer';
        }
        return result.pub.getSlicesBulkData([index])[0];
      };

      result.pub.getSlicesBulkData = function(indices) {
        var slicesBulkData;
        result.queueCommand('getSlicesBulkData', indices, function() { }, function(data) {
          for (var i = 0; i < data.length; i++) {
            for (var memberName in data[i]) {
              RT.assignPrototypes(data[i][memberName], result.members[memberName].type);
            }
          }
          slicesBulkData = data;
        });
        executeQueuedCommands();
        return slicesBulkData;
      };

      result.pub.getMemberBulkData = function(member) {
        if (typeof member !== 'string') {
          throw 'member: must be a string';
        }
        return result.pub.getMembersBulkData([member])[member];
      };

      result.pub.getMembersBulkData = function(members) {
        var membersBulkData;
        result.queueCommand('getMembersBulkData', members, function() { }, function(data) {
          for (var member in data) {
            var memberData = data[member];
            for (var i=0; i<memberData.length; ++i) {
              RT.assignPrototypes(memberData[i], result.members[member].type);
            }
          }
          membersBulkData = data;
        });
        executeQueuedCommands();
        return membersBulkData;
      };

      result.pub.setSlicesBulkData = function(data) {
        result.queueCommand('setSlicesBulkData', data);
      };

      result.pub.setSliceBulkData = function(sliceIndex, data) {
        result.queueCommand('setSlicesBulkData', [{
          sliceIndex: sliceIndex,
          data: data
        }]);
      };

      result.pub.getBulkDataJSON = function() {
        var json;
        result.queueCommand('getBulkDataJSON', null, function() {}, function(data) {
          json = data;
        });
        executeQueuedCommands();
        return json;
      };

      result.pub.setBulkDataJSON = function(data) {
        result.queueCommand('setBulkDataJSON', data);
      };

      result.pub.putResourceToFile = function(fileHandle, memberName) {
        result.queueCommand('putResourceToFile', {
          'memberName': memberName,
          'file': fileHandle
        });
        executeQueuedCommands();
      }
      return result;
    };

    DG.createNode = function(name) {
      var result = DG.createContainer(name);
      result.dependencies = {};
      result.bindings = DG.createBindingList([name, 'bindings']);

      var parentPatch = result.patch;
      result.patch = function(diff) {
        parentPatch(diff);

        if ('dependencies' in diff) {
          result.dependencies = {};
          for (var dependencyName in diff.dependencies) {
            var dependencyNodeName = diff.dependencies[dependencyName];
            result.dependencies[dependencyName] = DG.namedObjects[dependencyNodeName].pub;
          }
        }

        if ('bindings' in diff)
          result.bindings.patch(diff.bindings);
      };
      
      var evaluateAsyncFinishedSerial = 0;
      var evaluateAsyncFinishedCallbacks = {};

      var parentRoute = result.route;
      result.route = function(src, cmd, arg) {
        if (src.length == 1 && src[0] == 'bindings') {
          src.shift();
          result.bindings.route(src, cmd, arg);
        }
        else if (cmd == "evaluateAsyncFinished") {
          var callback = evaluateAsyncFinishedCallbacks[arg];
          delete evaluateAsyncFinishedCallbacks[arg];
          callback();
        }
        else
          parentRoute(src, cmd, arg);
      };

      result.pub.getType = function() {
        return 'Node';
      };

      result.pub.setDependency = function(dependencyNode, dependencyName) {
        try {
          if (typeof dependencyName !== 'string')
            throw 'must be a string';
          else if (dependencyName == '')
            throw 'must not be empty';
          else if (dependencyName == 'self')
            throw "must not be 'self'";
        }
        catch (e) {
          throw 'dependencyName: ' + e;
        }
        var oldDependency = result.dependencies[dependencyName];
        result.dependencies[dependencyName] = dependencyNode;
        result.queueCommand('setDependency', {
          'name': dependencyName,
          'node': dependencyNode.getName()
        }, function () {
          if (oldDependency)
            result.dependencies[dependencyName] = oldDependency;
          else delete result.dependencies[dependencyName];
        });
      };

      result.pub.getDependencies = function() {
        return result.dependencies;
      };

      result.pub.getDependency = function(name) {
        if (!(name in result.dependencies))
          throw "no dependency named '" + name + "'";
        return result.dependencies[name];
      };
      
      result.pub.removeDependency = function(dependencyName) {
        try {
          if (typeof dependencyName !== 'string')
            throw 'must be a string';
          else if (dependencyName == '')
            throw 'must not be empty';
          else if (dependencyName == 'self')
            throw "must not be 'self'";
        }
        catch (e) {
          throw 'dependencyName: ' + e;
        }
        var oldDependency = result.dependencies[dependencyName];
        delete result.dependencies[dependencyName];
        result.queueCommand('removeDependency', dependencyName, function () {
          if (oldDependency)
            result.dependencies[dependencyName] = oldDependency;
          else delete result.dependencies[dependencyName];
        });
      };

      result.pub.evaluate = function() {
        result.queueCommand('evaluate');
        executeQueuedCommands();
      };

      result.pub.evaluateAsync = function(callback) {
        var serial = evaluateAsyncFinishedSerial++;
        evaluateAsyncFinishedCallbacks[serial] = callback;
        result.queueCommand('evaluateAsync', serial);
        executeQueuedCommands();
      };

      result.pub.bindings = result.bindings.pub;

      return result;
    };

    DG.createResourceLoadNode = function(name) {
      var parentHandle,
        onloadSuccessCallbacks = [],
        onloadProgressCallbacks = [],
        onloadFailureCallbacks = [];

      var node = DG.createNode(name);

      parentHandle = node.handle;

      node.handle = function(cmd, arg) {
        var i;
        switch (cmd) {
          case 'resourceLoadSuccess':
            for (i = 0; i < onloadSuccessCallbacks.length; i++) {
              onloadSuccessCallbacks[i](node.pub);
            }
            break;
          case 'resourceLoadProgress':
            for (i = 0; i < onloadProgressCallbacks.length; i++) {
              onloadProgressCallbacks[i](node.pub, arg);
            }
            break;
          case 'resourceLoadFailure':
            for (i = 0; i < onloadFailureCallbacks.length; i++) {
              onloadFailureCallbacks[i](node.pub);
            }
            break;
          default:
            parentHandle(cmd, arg);
        }
      };

      node.pub.addOnLoadSuccessCallback = function(callback) {
        //At this 'core' level we don't try to detect same/different URLs or the fact that is it already loaded
        onloadSuccessCallbacks.push(callback);
      }
      node.pub.addOnLoadProgressCallback = function(callback) {
        //At this 'core' level we don't try to detect same/different URLs or the fact that is it already loaded
        onloadProgressCallbacks.push(callback);
      }
      node.pub.addOnLoadFailureCallback = function(callback) {
        onloadFailureCallbacks.push(callback);
      }
      return node;
    };

    DG.createEvent = function(name) {
      var result = DG.createContainer(name);

      var parentPatch = result.patch;
      result.patch = function(diff) {
        parentPatch(diff);

        if ('eventHandlers' in diff) {
          result.eventHandlers = [];
          for (var eventHandlerIndex in diff.eventHandlers) {
            var eventHandlerName = diff.eventHandlers[eventHandlerIndex];
            result.eventHandlers.push(DG.namedObjects[eventHandlerName].pub);
          }
        }
      };

      var parentHandleNotification = result.handle;
      result.handle = function(cmd, arg) {
        parentHandleNotification(cmd, arg);
      };

      result.pub.getType = function() {
        return 'Event';
      };

      result.pub.appendEventHandler = function(eventHandler) {
        result.queueCommand('appendEventHandler', eventHandler.getName());
        delete result.eventHandlers;
      };

      result.pub.getEventHandlers = function() {
        if (!('eventHandlers' in result))
          executeQueuedCommands();
        return result.eventHandlers;
      };

      result.pub.fire = function() {
        result.queueCommand('fire');
        executeQueuedCommands();
      };
                    
      var typeName;
      result.pub.setSelectType = function(tn) {
        var results = [];
        result.queueCommand('setSelectType', tn);
        executeQueuedCommands();
        typeName = tn;
        return results;
      };

      result.pub.select = function() {
        var results = [];
        result.queueCommand('select', typeName, function() { }, function(commandResults) {
          for (var i = 0; i < commandResults.length; ++i) {
            var commandResult = commandResults[i];
            results.push({
              node: DG.namedObjects[commandResult.node].pub,
              value: RT.assignPrototypes(commandResult.data, typeName)
            });
          }
        });
        executeQueuedCommands();
        return results;
      };
      
      return result;
    };

    DG.createEventHandler = function(name) {
      var result = DG.createContainer(name);
      result.scopes = {};
      result.preDescendBindings = DG.createBindingList([name, 'preDescendBindings']);
      result.postDescendBindings = DG.createBindingList([name, 'postDescendBindings']);

      var parentPatch = result.patch;
      result.patch = function(diff) {
        parentPatch(diff);

        if ('bindingName' in diff) {
          result.bindingName = diff.bindingName;
        }

        if ('childEventHandlers' in diff) {
          result.childEventHandlers = [];
          for (var childEventHandlerIndex in diff.childEventHandlers) {
            var childEventHandlerName = diff.childEventHandlers[childEventHandlerIndex];
            result.childEventHandlers.push(DG.namedObjects[childEventHandlerName].pub);
          }
        }

        if ('scopes' in diff) {
          result.scopes = {};
          for (var scopeName in diff.scopes) {
            var scopeNodeName = diff.scopes[scopeName];
            result.scopes[scopeName] = DG.namedObjects[scopeNodeName].pub;
          }
        }

        if ('preDescendBindings' in diff)
          result.preDescendBindings.patch(diff.preDescendBindings);

        if ('postDescendBindings' in diff)
          result.postDescendBindings.patch(diff.postDescendBindings);
      };

      var parentRoute = result.route;
      result.route = function(src, cmd, arg) {
        if (src.length == 1 && src[0] == 'preDescendBindings') {
          src.shift();
          result.preDescendBindings.route(src, cmd, arg);
        }
        else if (src.length == 1 && src[0] == 'postDescendBindings') {
          src.shift();
          result.postDescendBindings.route(src, cmd, arg);
        }
        else
          parentRoute(src, cmd, arg);
      };

      result.pub.getType = function() {
        return 'EventHandler';
      };

      result.pub.getScopeName = function() {
        return result.bindingName;
      };

      result.pub.setScopeName = function(bindingName) {
        var oldBindingName = result.bindingName;
        result.queueCommand('setScopeName', bindingName, function() {
          result.bindingName = oldBindingName;
        });
      };

      result.pub.appendChildEventHandler = function(childEventHandler) {
        var oldChildEventHandlers = result.childEventHandlers;
        delete result.childEventHandlers;
        result.queueCommand('appendChildEventHandler', childEventHandler.getName(), function() {
          result.childEventHandlers = oldChildEventHandlers;
        });
      };

      result.pub.removeChildEventHandler = function(childEventHandler) {
        var oldChildEventHandlers = result.childEventHandlers;
        delete result.childEventHandlers;
        result.queueCommand('removeChildEventHandler', childEventHandler.getName(), function() {
          result.childEventHandlers = oldChildEventHandlers;
        });
      };

      result.pub.getChildEventHandlers = function() {
        if (!('childEventHandlers' in result))
          executeQueuedCommands();
        return result.childEventHandlers;
      };

      result.pub.setScope = function(name, node) {
        try {
          if (typeof name !== 'string')
            throw 'must be a string';
          else if (name == '')
            throw 'must not be empty';
        }
        catch (e) {
          throw "name: " + e;
        }
        var oldNode = result.scopes[name];
        result.scopes[name] = node;
        result.queueCommand('setScope', {
          name: name,
          node: node.getName()
        }, function () {
          if (oldNode)
            result.scopes[name] = oldNode;
          else delete result.scopes[name];
        });
      };

      result.pub.removeScope = function(name) {
        try {
          if (typeof name !== 'string')
            throw 'must be a string';
          else if (name == '')
            throw 'must not be empty';
        }
        catch (e) {
          throw "name: " + e;
        }
        var oldNode = result.scopes[name];
        delete result.scopes[name];
        result.queueCommand('removeScope', name, function () {
          if (oldNode)
            result.scopes[name] = oldNode;
          else delete result.scopes[name];
        });
      };

      result.pub.getScopes = function() {
        return result.scopes;
      };

      result.pub.setSelector = function(targetName, binding) {
        var operatorName;
        try {
          operatorName = binding.getOperator().getName();
        }
        catch (e) {
          throw 'operator: not an operator';
        }

        result.queueCommand('setSelector', {
          targetName: targetName,
          operator: operatorName,
          parameterLayout: binding.getParameterLayout()
        });
      };

      result.pub.preDescendBindings = result.preDescendBindings.pub;
      result.pub.postDescendBindings = result.postDescendBindings.pub;

      return result;
    };

    DG.getOrCreateNamedObject = function(name, type) {
      if (!(name in DG.namedObjects)) {
        switch (type) {
          case 'Operator':
            DG.createOperator(name);
            break;
          case 'Node':
            DG.createNode(name);
            break;
          case 'Event':
            DG.createEvent(name);
            break;
          case 'EventHandler':
            DG.createEventHandler(name);
            break;
          default:
            throw "unhandled type '" + type + "'";
        }
      }
      return DG.namedObjects[name];
    };

    DG.handleStateNotification = function(state) {
      DG.namedObjects = {};
      for (var namedObjectName in state) {
        var namedObjectState = state[namedObjectName];
        DG.getOrCreateNamedObject(namedObjectName, namedObjectState.type);
      }
      for (var namedObjectName in state) {
        DG.namedObjects[namedObjectName].patch(state[namedObjectName]);
      }
    };

    DG.handle = function(cmd, arg) {
      switch (cmd) {
        case 'log':
          if (logCallback) {
            logCallback(arg);
          }
          break;
        default:
          throw "command '" + cmd + "': unrecognized";
      }
    };

    DG.route = function(src, cmd, arg) {
      if (src.length == 0) {
        DG.handle(cmd, arg);
      }
      else {
        var namedObjectName = src.shift();
        var namedObjectType;
        if (typeof arg === 'object' && 'type' in arg)
          namedObjectType = arg.type;
        DG.getOrCreateNamedObject(namedObjectName, namedObjectType).route(src, cmd, arg);
      }
    };

    DG.pub = {
      createOperator: function(name) {
        var operator = DG.createOperator(name);
        DG.queueCommand([], 'createOperator', name, function() {
          operator.destroy();
        });
        return operator.pub;
      },
      createNode: function(name) {
        var node = DG.createNode(name);
        DG.queueCommand([], 'createNode', name, function() {
          node.destroy();
        });
        return node.pub;
      },
      createResourceLoadNode: function(name) {
        var node = DG.createResourceLoadNode(name);
        DG.queueCommand([], 'createResourceLoadNode', name, function() {
          node.destroy();
        });
        return node.pub;
      },
      createEvent: function(name) {
        var event = DG.createEvent(name);
        DG.queueCommand([], 'createEvent', name, function() {
          event.destroy();
        });
        return event.pub;
      },
      createEventHandler: function(name) {
        var eventHandler = DG.createEventHandler(name);
        DG.queueCommand([], 'createEventHandler', name, function() {
          eventHandler.destroy();
        });
        return eventHandler.pub;
      },

      createBinding: function() {
        var binding = DG.createBinding();
        return binding.pub;
      },

      getAllNamedObjects: function() {
        var result = {};
        for (var namedObjectName in DG.namedObjects) {
          result[namedObjectName] = DG.namedObjects[namedObjectName].pub;
        }
        return result;
      }
    };

    return DG;
  };
  var DG = createDG();

  var createEX = function() {
    var EX = {
      loadedExts: {},
      pub: {}
    };

    EX.patch = function(diff) {
      for (var name in diff) {
        if (diff[name])
          EX.loadedExts[name] = diff[name];
        else
          delete EX.loadedExts[name];
      }
    };

    EX.handleStateNotification = function(state) {
      EX.loadedExts = {};
      EX.patch(state);
    };

    EX.handle = function(cmd, arg) {
      switch (cmd) {
        case 'delta':
          EX.patch(arg);
          break;
        default:
          throw "command '" + cmd + "': unrecognized";
      }
    };

    EX.route = function(src, cmd, arg) {
      if (src.length == 0) {
        EX.handle(cmd, arg);
      }
      else
        throw 'unroutable';
    };

    EX.pub.getLoadedExts = function() {
      return EX.loadedExts;
    };

    return EX;
  };
  var EX = createEX();

  var createIO = function() {

    var IO = {
      pub: {
        forOpen: 'openMode',
        forOpenWithWriteAccess: 'openWithWriteAccessMode',
        forSave: 'saveMode'
      }
    };

    IO.queueCommand = function(cmd, arg, unwind, callback) {
      queueCommand(['IO'], cmd, arg, unwind, callback);
    };

    var queryUserFile = function(funcname, mode, uiTitle, extension, defaultFileName) {
      if(mode !== IO.pub.forOpen && mode !== IO.pub.forOpenWithWriteAccess && mode !== IO.pub.forSave)
        throw 'Invalid mode: \"' + mode + '\': can be IO.forOpen, IO.forOpenWithWriteAccess or IO.forSave';
      var result = {};
      IO.queueCommand(funcname, {
        'existingFile': mode === IO.pub.forOpenWithWriteAccess || mode === IO.pub.forOpen,
        'writeAccess': mode === IO.pub.forOpenWithWriteAccess || mode === IO.pub.forSave,
        'uiOptions': {
          'title': uiTitle,
          'extension': extension,
          'defaultFileName': defaultFileName
        }
      }, function() {
      }, function(data) {
        result = data;
      });
      executeQueuedCommands();
      return result;
    };

    IO.pub.queryUserFileAndFolderHandle = function(mode, uiTitle, extension, defaultFileName) {
      return queryUserFile('queryUserFileAndFolder', mode, uiTitle, extension, defaultFileName);
    };

    IO.pub.queryUserFileHandle = function(mode, uiTitle, extension, defaultFileName) {
      return queryUserFile('queryUserFile', mode, uiTitle, extension, defaultFileName);
    };

    IO.pub.getTextFileContent = function(handle) {
      var fileContent;
      IO.queueCommand('getTextFileContent', handle, function() {
      }, function(data) {
        fileContent = data;
      });
      executeQueuedCommands();
      return fileContent;
    }

    IO.pub.putTextFileContent = function(handle, content, append) {
      IO.queueCommand('putTextFileContent', {
        'content': content,
        'file': handle,
        'append': append
      });
      executeQueuedCommands();
    }

    IO.pub.buildFileHandleFromRelativePath = function(handle) {
      var handle;
      IO.queueCommand('createFileHandleFromRelativePath', handle, function() {
      }, function(data) {
        handle = data;
      });
      executeQueuedCommands();
      return handle;
    }

    IO.pub.buildFolderHandleFromRelativePath = function(handle) {
      var handle;
      IO.queueCommand('createFolderHandleFromRelativePath', handle, function() {
      }, function(data) {
        handle = data;
      });
      executeQueuedCommands();
      return handle;
    }

    IO.pub.getFileHandleInfo = function(handle) {
      var handle;
      IO.queueCommand('getFileInfo', handle, function() {
      }, function(data) {
        handle = data;
      });
      executeQueuedCommands();
      return handle;
    }

    return IO;
  };
  var IO = createIO();

  var createBuild = function() {
    var build = {
      pub: {}
    };

    build.patch = function(diff) {
      for (var name in diff) {
        build[name] = diff[name];
      }
    };

    build.handleStateNotification = function(state) {
      build.patch(state);
    };

    build.handle = function(cmd, arg) {
      switch (cmd) {
        case 'delta':
          build.patch(arg);
          break;
        default:
          throw "command '" + cmd + "': unrecognized";
      }
    };

    build.route = function(src, cmd, arg) {
      if (src.length == 0) {
        build.handle(cmd, arg);
      }
      else
        throw 'unroutable';
    };

    build.pub.isExpired = function() {
      return build.isExpired;
    };

    build.pub.getName = function() {
      return build.name;
    };

    build.pub.getPureVersion = function() {
      return build.pureVersion;
    };

    build.pub.getFullVersion = function() {
      return build.fullVersion;
    };

    build.pub.getDesc = function() {
      return build.desc;
    };

    build.pub.getCopyright = function() {
      return build.copyright;
    };

    build.pub.getURL = function() {
      return build.url;
    };

    build.pub.getOS = function() {
      return build.os;
    };

    build.pub.getArch = function() {
      return build.arch;
    };

    return build;
  };
  var build = createBuild();

  var state = {
  };

  var patch = function(diff) {
    if ('licenses' in diff)
      state.licenses = diff.licenses;
    if ('contextID' in diff)
      state.contextID = diff.contextID;
  };
  
  var GC = (function (namespace) {
    var nextID = 0;
    var objects = {};
    
    return {
      createObject: function (namespace) {
        var id = "GC_" + nextID++;
        
        var result = {
          id: id,
          nextCallbackID: 0,
          callbacks: {},
          queueCommand: function (cmd, arg, unwind, callback) {
            if (!this.id)
              throw "GC object has already been disposed";
            queueCommand([namespace, this.id], cmd, arg, unwind, callback);
          },
          registerCallback: function (callback) {
            var callbackID = this.nextCallbackID++;
            this.callbacks[callbackID] = callback;
            return callbackID;
          },
          route: function(src, cmd, arg) {
            var callback = this.callbacks[arg.serial];
            delete this.callbacks[arg.serial];
            callback(arg.result);
          },
          pub: {
          }
        };
        
        result.pub.getID = function () {
          return result.id;
        };
        
        result.pub.dispose = function () {
          result.queueCommand('dispose');
          delete objects[id];
          delete result.id;
        };
        
        objects[id] = result;
        
        return result;
      },
      
      route: function (src, cmd, arg) {
        var id = src.shift();
        var object = objects[id];
        object.route(src, cmd, arg);
      }
    };
  })();

  var KLC = (function() {
    var KLC = {
    };

    var populateFunction = function (function_) {
      function_.pub.getDiagnostics = function () {
        var diagnostics;
        function_.queueCommand('getDiagnostics', null, null, function (result) {
          diagnostics = result;
        });
        executeQueuedCommands();
        return diagnostics;
      };

      function_.pub.toJSON = function () {
        var json;
        function_.queueCommand('toJSON', null, null, function (result) {
          json = result;
        });
        executeQueuedCommands();
        return json;
      };
    };

    var populateOperator = function (operator) {
      populateFunction(operator);
    };

    var populateArrayMapOperator = function (operator) {
      populateOperator(operator);
    };

    var populateArrayGeneratorOperator = function (arrayGeneratorOperator) {
      populateOperator(arrayGeneratorOperator);
    };

    var populateValueGeneratorOperator = function (operator) {
      populateOperator(operator);
    };

    var populateValueMapOperator = function (operator) {
      populateOperator(operator);
    };

    var populateValueTransformOperator = function (operator) {
      populateOperator(operator);
    };

    var populateReduceOperator = function (reduceOperator) {
      populateOperator(reduceOperator);
    };

    var populateExecutable = function (executable) {
      executable.pub.getAST = function () {
        var ast;
        executable.queueCommand('getAST', null, null, function (result) {
          ast = result;
        });
        executeQueuedCommands();
        return ast;
      };
      
      executable.pub.getDiagnostics = function () {
        var diagnostics;
        executable.queueCommand('getDiagnostics', null, null, function (result) {
          diagnostics = result;
        });
        executeQueuedCommands();
        return diagnostics;
      };
      
      executable.pub.resolveArrayMapOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateArrayMapOperator(operator);
        executable.queueCommand('resolveArrayMapOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveReduceOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateReduceOperator(operator);
        executable.queueCommand('resolveReduceOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveArrayGeneratorOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateArrayGeneratorOperator(operator);
        executable.queueCommand('resolveArrayGeneratorOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveValueGeneratorOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateValueGeneratorOperator(operator);
        executable.queueCommand('resolveValueGeneratorOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveValueMapOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateValueMapOperator(operator);
        executable.queueCommand('resolveValueMapOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveValueTransformOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateValueTransformOperator(operator);
        executable.queueCommand('resolveValueTransformOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
      
      executable.pub.resolveArrayTransformOperator = function (operatorName) {
        var operator = GC.createObject('KLC');
        populateArrayTransformOperator(operator);
        executable.queueCommand('resolveArrayTransformOperator', {
          id: operator.id,
          operatorName: operatorName
        }, function () {
          delete operator.id;
        });
        return operator.pub;
      };
    };
    
    KLC.pub = {
      createCompilation: function (sourceName, sourceCode) {
        var compilation = GC.createObject('KLC');
        
        compilation.sourceCodes = {
        };
        
        compilation.pub.addSource = function (sourceName, sourceCode) {
          var oldSourceCode = compilation.sourceCodes[sourceName];
          compilation.sourceCodes[sourceName] = sourceCode;
          compilation.queueCommand('addSource', {
            sourceName: sourceName,
            sourceCode: sourceCode
          }, function () {
            compilation.sourceCodes[sourceName] = oldSourceCode;
          });
        };
        
        compilation.pub.removeSource = function (sourceName) {
          var oldSourceCode = compilation.sourceCodes[sourceName];
          delete compilation.sourceCodes[sourceName];
          compilation.queueCommand('removeSource', {
            sourceName: sourceName,
          }, function () {
            compilation.sourceCodes[sourceName] = oldSourceCode;
          });
        };
        
        compilation.pub.getSources = function () {
          var sources;
          compilation.queueCommand('getSources', null, null, function (result) {
            sources = result;
          });
          executeQueuedCommands();
          return sources;
        };
        
        compilation.pub.run = function () {
          var executable = GC.createObject('KLC');
          
          populateExecutable(executable);
          
          compilation.queueCommand('run', {
            id: executable.id
          }, function () {
            delete executable.id;
          });
          
          return executable.pub;
        };
        
        var arg = {
          id: compilation.id
        };
        if (sourceName != undefined)
          arg.sourceName = sourceName;
        if (sourceCode != undefined)
          arg.sourceCode = sourceCode;
        
        queueCommand(['KLC'],'createCompilation', arg, function () {
          delete compilation['id'];
        });
        
        return compilation.pub;
      },
      
      createExecutable: function (sourceName, sourceCode) {
        var executable = GC.createObject('KLC');
          
        populateExecutable(executable);
          
        var arg = {
          id: executable.id,
          sourceName: sourceName,
          sourceCode: sourceCode
        };
        
        queueCommand(['KLC'],'createExecutable', arg, function () {
          delete executable['id'];
        });
        
        return executable.pub;
      },
      
      createArrayMapOperator: function (sourceName, sourceCode, operatorName) {
        var operator = GC.createObject('KLC');
          
        populateArrayMapOperator(operator);
          
        var arg = {
          id: operator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createArrayMapOperator', arg, function () {
          delete operator['id'];
        });
        
        return operator.pub;
      },
      
      createArrayGeneratorOperator: function (sourceName, sourceCode, operatorName) {
        var arrayGeneratorOperator = GC.createObject('KLC');
          
        populateArrayGeneratorOperator(arrayGeneratorOperator);
          
        var arg = {
          id: arrayGeneratorOperator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createArrayGeneratorOperator', arg, function () {
          delete arrayGeneratorOperator['id'];
        });
        
        return arrayGeneratorOperator.pub;
      },
      
      createValueGeneratorOperator: function (sourceName, sourceCode, operatorName) {
        var operator = GC.createObject('KLC');
          
        populateValueGeneratorOperator(operator);
          
        var arg = {
          id: operator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createValueGeneratorOperator', arg, function () {
          delete operator.id;
        });
        
        return operator.pub;
      },
      
      createValueMapOperator: function (sourceName, sourceCode, operatorName) {
        var operator = GC.createObject('KLC');
          
        populateValueMapOperator(operator);
          
        var arg = {
          id: operator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createValueMapOperator', arg, function () {
          delete operator.id;
        });
        
        return operator.pub;
      },
      
      createValueTransformOperator: function (sourceName, sourceCode, operatorName) {
        var operator = GC.createObject('KLC');
          
        populateValueTransformOperator(operator);
          
        var arg = {
          id: operator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createValueTransformOperator', arg, function () {
          delete operator.id;
        });
        
        return operator.pub;
      },
      
      createArrayTransformOperator: function (sourceName, sourceCode, operatorName) {
        var operator = GC.createObject('KLC');
          
        populateArrayMapOperator(operator);
          
        var arg = {
          id: operator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createArrayTransformOperator', arg, function () {
          delete operator.id;
        });
        
        return operator.pub;
      },
      
      createReduceOperator: function (sourceName, sourceCode, operatorName) {
        var reduceOperator = GC.createObject('KLC');
          
        populateReduceOperator(reduceOperator);
          
        var arg = {
          id: reduceOperator.id,
          sourceName: sourceName,
          sourceCode: sourceCode,
          operatorName: operatorName
        };
        
        queueCommand(['KLC'],'createReduceOperator', arg, function () {
          delete reduceOperator['id'];
        });
        
        return reduceOperator.pub;
      },
    };

    return KLC;
  })();

  var MR = (function() {
    var MR = {
    };
    
    var populateProducer = function (producer) {
      producer.pub.toJSON = function () {
        var jsonDesc;
        producer.queueCommand('toJSON', null, null, function (result) {
          jsonDesc = result;
        });
        executeQueuedCommands();
        return jsonDesc;
      };
    };
    
    var populateValueProducer = function (valueProducer) {
      populateProducer(valueProducer);
      
      valueProducer.pub.produce = function () {
        var result;
        valueProducer.queueCommand('produce', null, null, function (_) {
          result = _;
        });
        executeQueuedCommands();
        return result;
      };
      
      valueProducer.pub.produceAsync = function (callback) {
        valueProducer.queueCommand('produceAsync', valueProducer.registerCallback(callback));
        executeQueuedCommands();
      };

      valueProducer.pub.flush = function () {
        valueProducer.queueCommand('flush');
      };
    };

    var populateConstValue = function (constValue) {
      populateValueProducer(constValue);
    };
    
    var populateReduce = function (reduce) {
      populateValueProducer(reduce);
    };
    
    var populateValueGenerator = function (valueGenerator) {
      populateValueProducer(valueGenerator);
    };
    
    var populateValueGenerator = function (valueGenerator) {
      populateValueProducer(valueGenerator);
    };
    
    var populateValueMap = function (valueMap) {
      populateValueProducer(valueMap);
    };
    
    var populateValueTransform = function (valueTransform) {
      populateValueProducer(valueTransform);
    };
    
    var populateValueCache = function (valueCache) {
      populateValueProducer(valueCache);
    };
    
    var populateArrayProducer = function (arrayProducer) {
      populateProducer(arrayProducer);
      
      arrayProducer.pub.getCount = function () {
        var count;
        arrayProducer.queueCommand('getCount', null, null, function (result) {
          count = result;
        });
        executeQueuedCommands();
        return count;
      };
      
      arrayProducer.pub.produce = function (index, count) {
        var result;
        
        var arg = {};
        if (index !== undefined) {
          if (count !== undefined)
            arg.count = count;
          arg.index = index;
        }
        arrayProducer.queueCommand('produce', arg, null, function (_) {
          result = _;
        });
        executeQueuedCommands();
        return result;
      };
      
      arrayProducer.pub.produceAsync = function () {
        var arg = {};
        var callback;
        switch (arguments.length) {
          case 1:
            callback = arguments[0];
            break;
          case 2:
            arg.index = arguments[0];
            callback = arguments[1];
            break;
          case 3:
            arg.index = arguments[0];
            arg.count = arguments[1];
            callback = arguments[2];
            break;
          default:
            throw "produceAsync: invalid arguments";
        }
        arg.serial = arrayProducer.registerCallback(callback);
        arrayProducer.queueCommand('produceAsync', arg);
        executeQueuedCommands();
      };

      arrayProducer.pub.flush = function () {
        arrayProducer.queueCommand('flush');
      };
    };
    
    var populateConstArray = function (constArray) {
      populateArrayProducer(constArray);
    };
    
    var populateArrayGenerator = function (arrayGenerator) {
      populateArrayProducer(arrayGenerator);
    };
    
    var populateArrayMap = function (object) {
      populateArrayProducer(object);
    };
    
    var populateArrayTransform = function (object) {
      populateArrayProducer(object);
    };

    var populateArrayCache = function (object) {
      populateArrayProducer(object);
    };
    
    MR.pub = {
      createArrayMap: function(input, operator, shared) {
        var result = GC.createObject('MR');
        
        populateArrayMap(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID(),
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createArrayMap', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createReduce: function(inputArrayProducer, reduceOperator, sharedValueProducer) {
        var reduce = GC.createObject('MR');
        
        populateReduce(reduce);
        
        var arg = {
          id: reduce.id,
          inputID: inputArrayProducer.getID(),
          operatorID: reduceOperator.getID()
        };
        if (sharedValueProducer)
          arg.sharedID = sharedValueProducer.getID();
        
        queueCommand(['MR'], 'createReduce', arg, function () {
          delete reduce.id;
        });
        return reduce.pub;
      },
      
      createValueGenerator: function(operator, shared) {
        var result = GC.createObject('MR');
        
        populateValueGenerator(result);
        
        var arg = {
          id: result.id,
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createValueGenerator', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createValueMap: function(input, operator, shared) {
        var result = GC.createObject('MR');
        
        populateValueMap(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID(),
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createValueMap', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createValueTransform: function(input, operator, shared) {
        var result = GC.createObject('MR');
        
        populateValueTransform(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID(),
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createValueTransform', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createArrayTransform: function(input, operator, shared) {
        var result = GC.createObject('MR');
        
        populateArrayTransform(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID(),
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createArrayTransform', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createArrayGenerator: function(count, operator, shared) {
        var result = GC.createObject('MR');
        
        populateArrayGenerator(result);
        
        var arg = {
          id: result.id,
          countID: count.getID(),
          operatorID: operator.getID()
        };
        if (shared)
          arg.sharedID = shared.getID();
        
        queueCommand(['MR'], 'createArrayGenerator', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createArrayCache: function(input) {
        var result = GC.createObject('MR');
        
        populateArrayCache(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID()
        };
        
        queueCommand(['MR'], 'createArrayCache', arg, function () {
          delete result.id;
        });
        return result.pub;
      },
      
      createConstArray: function(elementType, data) {
        var constArray = GC.createObject('MR');
        
        populateConstArray(constArray);
        
        var arg = {
          id: constArray.id
        };
        if (typeof elementType === "string") {
          arg.elementType = elementType;
          arg.data = data;
        }
        else if (typeof elementType === "object") {
          var inputArg = elementType;
          arg.elementType = inputArg.elementType;
          if (inputArg.data)
            arg.data = inputArg.data;
          if (inputArg.jsonData)
            arg.jsonData = inputArg.jsonData;
        }
        else throw "createConstArray: first argumenet must be string or object";
        
        queueCommand(['MR'], 'createConstArray', arg, function () {
          delete constArray.id;
        });
        return constArray.pub;
      },
      
      createConstValue: function(valueType, data) {
        var constValue = GC.createObject('MR');
        
        populateConstValue(constValue);
        
        queueCommand(['MR'], 'createConstValue', {
          id: constValue.id,
          valueType: valueType,
          data: data
        }, function () {
          delete constValue.id;
        });
        return constValue.pub;
      },

      createValueCache: function(input) {
        var result = GC.createObject('MR');
        
        populateValueCache(result);
        
        var arg = {
          id: result.id,
          inputID: input.getID()
        };
        
        queueCommand(['MR'], 'createValueCache', arg, function () {
          delete result.id;
        });
        return result.pub;
      }
    };

    return MR;
  })();

  var createVP = function() {
    var VP = {
      viewPorts: {},
      pub: {}
    };

    VP.createViewPort = function(name) {
      var viewPort = {
        popUpMenuItems: {}
      };

      viewPort.patch = function(diff) {
        if ('width' in diff)
          viewPort.width = diff.width;

        if ('height' in diff)
          viewPort.height = diff.height;

        if ('windowNode' in diff)
          viewPort.windowNode = DG.namedObjects[diff.windowNode].pub;

        if ('redrawEvent' in diff)
          viewPort.redrawEvent = DG.namedObjects[diff.redrawEvent].pub;
      };

      viewPort.handle = function(cmd, arg) {
        switch (cmd) {
          case 'delta':
            viewPort.patch(arg);
            break;
          case 'redrawFinished':
            if (viewPort.redrawFinishedCallback)
              viewPort.redrawFinishedCallback();
            break;
          case 'popUpMenuItemSelected':
            if (arg in viewPort.popUpMenuItems)
              viewPort.popUpMenuItems[arg]();
            break;
          default:
            throw "command '" + cmd + "': unrecognized";
        }
      };

      viewPort.route = function(src, cmd, arg) {
        if (src.length == 0) {
          viewPort.handle(cmd, arg);
        }
        else
          throw 'unroutable';
      };

      viewPort.handleStateNotification = function(state) {
        viewPort.patch(state);
      };

      viewPort.queueCommand = function(cmd, arg, unwind, callback) {
        queueCommand(['VP', name], cmd, arg, unwind, callback);
      };

      viewPort.pub = {
        getName: function() {
          return name;
        },
        getWidth: function() {
          executeQueuedCommands();
          return viewPort.width;
        },
        getHeight: function() {
          executeQueuedCommands();
          return viewPort.height;
        },
        getFPS: function() {
          var fps = 0.0;
          viewPort.queueCommand('getFPS', null, function() {
          }, function(result) {
            fps = result;
          });
          executeQueuedCommands();
          return fps;
        },
        getWindowNode: function() {
          executeQueuedCommands();
          return viewPort.windowNode;
        },
        getRedrawEvent: function() {
          executeQueuedCommands();
          return viewPort.redrawEvent;
        },
        needsRedraw: function() {
          viewPort.queueCommand('needsRedraw');
          executeQueuedCommands();
        },
        setRedrawFinishedCallback: function(callback) {
          viewPort.redrawFinishedCallback = callback;
        },
        addPopUpMenuItem: function(name, desc, callback) {
          viewPort.popUpMenuItems[name] = callback;
          viewPort.queueCommand('addPopUpMenuItem', {
            desc: desc,
            arg: name
          });
        }
      };

      return viewPort;
    };

    VP.getOrCreateViewPort = function(viewPortName) {
      var viewPort = VP.viewPorts[viewPortName];
      if (!viewPort) {
        viewPort = VP.createViewPort(viewPortName);
        VP.viewPorts[viewPortName] = viewPort;
        VP.pub[viewPortName] = viewPort.pub;
      }
      return viewPort;
    };

    VP.handleStateNotification = function(newState) {
      for (var viewPortName in newState) {
        var viewPort = VP.getOrCreateViewPort(viewPortName);
        viewPort.handleStateNotification(newState[viewPortName]);
      }
    };

    VP.route = function(src, cmd, arg) {
      if (src.length > 0) {
        var viewPortName = src.shift();
        var viewPort = VP.getOrCreateViewPort(viewPortName);
        viewPort.route(src, cmd, arg);
      }
      else {
        VP.handle(cmd, arg);
      }
    };

    return VP;
  };
  var VP = createVP();

  var handleStateNotification = function(newState) {
    state = {};
    patch(newState);
    if ('build' in newState)
      build.handleStateNotification(newState.build);
    DG.handleStateNotification(newState.DG);
    RT.handleStateNotification(newState.RT);
    EX.handleStateNotification(newState.EX);
    if ('VP' in newState)
      VP.handleStateNotification(newState.VP);
  };

  var handle = function(cmd, arg) {
    try {
      switch (cmd) {
        case 'state':
          handleStateNotification(arg);
          break;
        default:
          throw 'unknown command';
      }
    }
    catch (e) {
      throw "command '" + cmd + "': " + e;
    }
  };

  var route = function(src, cmd, arg) {
    if (src.length == 0)
      handle(cmd, arg);
    else {
      var firstSrc = src.shift();

      switch (firstSrc) {
        case 'RT':
          RT.route(src, cmd, arg);
          break;
        case 'DG':
          DG.route(src, cmd, arg);
          break;
        case 'EX':
          EX.route(src, cmd, arg);
          break;
        case 'VP':
          VP.route(src, cmd, arg);
          break;
        case 'GC':
          GC.route(src, cmd, arg);
          break;
        default:
          throw 'unroutable';
      }
    }
  };

  fabricClient.setJSONNotifyCallback(function(jsonEncodedNotifications) {
    if (debugLogCallback)
      debugLogCallback('!! ' + jsonEncodedNotifications);

    var notifications;
    try {
      notifications = JSON.parse(jsonEncodedNotifications);
    }
    catch (e) {
      throw 'unable to parse JSON notifications';
    }

    var size = notifications.length;
    for (var i = 0; i < size; ++i) {
      var notification = notifications[i];
      var src = notification.src;
      var cmd = notification.cmd;
      var arg = notification.arg;
      route(src, cmd, arg);
    }
  });

  return {
    build: build.pub,
    RT: RT.pub,
    RegisteredTypesManager: RT.pub,
    DG: DG.pub,
    EX: EX.pub,
    IO: IO.pub,
    DependencyGraph: DG.pub,
    KLC: KLC.pub,
    MR: MR.pub,
    VP: VP.pub,
    getLicenses: function() {
      return state.licenses;
    },
    getContextID: function() {
      return state.contextID;
    },
    flush: function() {
      executeQueuedCommands();
    },
    close: function() {
      fabricClient.close();
    },
    getMemoryUsage: function() {
      var memoryUsage;
      queueCommand([], "getMemoryUsage", null, function () {
      }, function (result) {
        memoryUsage = result;
      });
      executeQueuedCommands();
      return memoryUsage;
    }
  };
}
);})();
