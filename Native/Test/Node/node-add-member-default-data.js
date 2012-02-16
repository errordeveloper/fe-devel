FABRIC = require('Fabric').createClient();

var node = FABRIC.DG.createNode("node");
node.addMember("foo","Scalar",3.141);
console.log(node.getData("foo", 0));
node.setData("foo", 0, 2.718);
console.log(node.getData("foo", 0));
node.resize(2);
console.log(node.getData("foo", 1));

FABRIC.close();
