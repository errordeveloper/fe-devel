F = require('Fabric').createClient();

var mapNamedObjectsToNames = function (namedObjects) {
  var result = [];
  for (var i=0; i<namedObjects.length; ++i)
    result.push(namedObjects[i].getName());
  return result;
};

e = F.DG.createEvent("event");
console.log(e.getName());
console.log(e.getType());

console.log(JSON.stringify(mapNamedObjectsToNames(e.getEventHandlers())));
eh = F.DG.createEventHandler("eventHandler");
e.appendEventHandler(eh);
console.log(JSON.stringify(mapNamedObjectsToNames(e.getEventHandlers())));

e.fire();

F.close();
