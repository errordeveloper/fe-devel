fabricClient = require('Fabric').createClient();

op = fabricClient.DG.createOperator("op");
op.setSourceCode("(inline)", "\
operator foo(\n\
  io Float64 input,\n\
  io Float64 output\n\
  ) {\n\
  output = input * input;\n\
}\n\
");
op.setEntryPoint("foo");

binding = fabricClient.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout([
  "dep.value",
  "self.value"
  ]);

depNode = fabricClient.DG.createNode("dep1");
depNode.addMember("value", "Float64");
depNode.setData("value", 0, 3.1623);

node = fabricClient.DG.createNode("node");
node.setDependency(depNode, "dep");
node.addMember("value", "Float64");
node.bindings.append(binding);
node.evaluate();
console.log(node.getData("value", 0));

depNode.destroy();
console.log(node.getErrors());

depNode = fabricClient.DG.createNode("dep2");
depNode.addMember("value", "Float64");
depNode.setData("value", 0, 1.4142);
node.setDependency(depNode, "dep");
node.evaluate();
console.log(node.getData("value", 0));

fabricClient.close();
