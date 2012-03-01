/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

eh = F.DG.createEventHandler("event handler");
console.log(eh.getName());
console.log(eh.getType());

var mapNamedObjectsToNames = function (namedObjects) {
  var result = [];
  for (var i=0; i<namedObjects.length; ++i)
    result.push(namedObjects[i].getName());
  return result;
};


console.log(JSON.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))));
var childEventHandler = F.DG.createEventHandler("child event handler");
eh.appendChildEventHandler(childEventHandler);
console.log(JSON.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))));
eh.appendChildEventHandler(F.DG.createEventHandler("another child event handler"));
console.log(JSON.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))));
eh.removeChildEventHandler(childEventHandler);
console.log(JSON.stringify((mapNamedObjectsToNames(eh.getChildEventHandlers()))));

F.close();
