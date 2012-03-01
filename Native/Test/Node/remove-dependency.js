/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

parentNode = F.DG.createNode('parentNode');
childNode = F.DG.createNode('childNode');
childNode.setDependency(parentNode, 'parent');
console.log((function (dependencies) {
  var result = [];
  for (var dependency in dependencies)
    result.push(dependency);
  return result;
})(childNode.getDependencies()));
try {
  childNode.removeDependency('notAParent');
  F.flush();
}
catch (e) {
  console.log("childNode.removeDependency('notAParent'): " + e);
}
childNode.removeDependency('parent');
console.log(childNode.getDependencies());

F.close();
