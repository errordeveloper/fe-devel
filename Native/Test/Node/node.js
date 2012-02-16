F = require('Fabric').createClient();

var mapNamedObjectsToNames = function (namedObjects) {
  var result = {};
  for (var i in namedObjects)
    result[i] = namedObjects[i].getName();
  return result;
};

node = F.DG.createNode("node");
console.log(node.getType());
node.addMember("foo", "String");
console.log(node.getMembers());
node.resize(2);
console.log(node.size());
node.setData("foo", 0, "fred");
node.setData("foo", 1, "joe");
console.log(node.getData("foo", 0));
console.log(node.getData("foo", 1));

node2 = F.DG.createNode("node2");
node.setDependency( node2, "parent" );
console.log((mapNamedObjectsToNames(node.getDependencies())));

F.close();
