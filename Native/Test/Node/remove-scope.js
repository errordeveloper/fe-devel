/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

node = F.DG.createNode('node');
eventHandler = F.DG.createEventHandler('eventHandler');
eventHandler.setScope('node', node);
console.log((function (scopes) {
  var result = [];
  for (var scope in scopes)
    result.push(scope);
  return result;
})(eventHandler.getScopes()));
try {
  eventHandler.removeScope('notANode');
  F.flush();
}
catch (e) {
  console.log("eventHandler.removeScope('notANode'): " + e);
}
eventHandler.removeScope('node');
console.log(eventHandler.getScopes());

F.close();
