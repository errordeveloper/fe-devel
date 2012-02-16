import fabric
fabricClient = fabric.createClient()

node = fabricClient.DependencyGraph.createNode("testDGNode");
node.addMember("intMember", "Integer");
node.addMember("stringMember", "String");

try:
  node.addMember("containerMember", "Container");
  node.evaluate();
except Exception as e:
  print("Runtime eval error (container member): " + str(e));

op = fabricClient.DependencyGraph.createOperator("op");
op.setEntryFunctionName("op");
op.setSourceCode('\
operator op(\n\
  io Container c,\n\
  io Integer i<>,\n\
  io String s<>\n\
  )\n\
{\n\
  report "Container string: " + c + " Count: " + c.size() + " Is valid: " + Boolean(c);\n\
  Container otherC;\n\
  report "Uninitialized Container string: " + otherC + " Is valid: " + Boolean(otherC);\n\
  report "Member sizes: " + i.size;\n\
  report "Members: " + i + " " + s;\n\
  otherC = c;\n\
  otherC.resize(Size(3));\n\
  report "Container string: " + otherC + " Count: " + otherC.size + " Is valid: " + Boolean(otherC);\n\
  report "To fix: i.size is wrong! LLVM doesn\'t know about aliasing!";\n\
  i[2] = 1;\n\
  s[2] = "test";\n\
  report "Member sizes: " + s.size;\n\
  report "Members: " + i + " " + s;\n\
  //Now test accessing uninitialized Container\n\
  Container bad;\n\
  report bad.size();\n\
}\n\
');
if len( op.getDiagnostics() ) > 0:
  print(fabric.stringify(op.getDiagnostics()));

opBinding = fabricClient.DG.createBinding();
opBinding.setOperator(op);
opBinding.setParameterLayout([
  "self",
  "self.intMember<>",
  "self.stringMember<>"
]);
node.bindings.append(opBinding);
try:
  node.evaluate();
except Exception as e:
  print("Runtime eval error (uninit container access): " + str(e) );

# Error test: we are not allowed to have io Container along with members elements
badOp = fabricClient.DependencyGraph.createOperator("badOp");
badOp.setEntryFunctionName("badOp");
badOp.setSourceCode('\
operator badOp(\n\
  io Container c,\n\
  io Integer i\n\
  )\n\
{\n\
}\n\
');
if len( badOp.getDiagnostics() ) > 0:
  print(fabric.stringify(badOp.getDiagnostics()));

badOpBinding = fabricClient.DG.createBinding();
badOpBinding.setOperator(badOp);
badOpBinding.setParameterLayout([
  "self",
  "self.intMember"
]);
node.bindings.append(badOpBinding);
if len( node.getErrors() ) > 0:
  print("Bad bindings error: " + fabric.stringify((node.getErrors())));

fabricClient.close();
