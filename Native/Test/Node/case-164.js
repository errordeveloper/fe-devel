/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

operator = F.DG.createOperator("op");
operator.setEntryFunctionName("entry");
operator.setSourceCode("\
operator entry( io Size foo ) {\n\
  report 'foo = ' + foo;\n\
}\n\
");
if (operator.getDiagnostics().length > 0 )
  console.log(operator.getDiagnostics());

binding = F.DependencyGraph.createBinding();
binding.setOperator(operator);
binding.setParameterLayout(['self.bar','self.foo']);

node = F.DG.createNode("node");
node.addMember('bar', 'Size');
node.setData('bar', 0, 42);
node.bindings.append(binding);
if (node.getErrors().length > 0)
  console.log(node.getErrors());
try {
  node.evaluate();
}
catch (e) {
  console.log(e);
}

F.close();
