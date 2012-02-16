// test that setting slice count to 'undefined' gives JS exception
// and not a crash
F = require('Fabric').createClient();
var node = F.DG.createNode("foo");
node.setCount(undefined);
node.evaluate();
F.close();
