/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

o = F.DG.createOperator("op");
o.setSourceCode("operator entry( io Scalar x, io Scalar y ) { y = x * x; }");
o.setEntryFunctionName("entry");

b = F.DG.createBinding();
b.setOperator(o);
b.setParameterLayout(["self.x","self.y"]);

n = F.DG.createNode("node");
n.addMember("x", "Scalar");
n.addMember("y", "Scalar");
n.bindings.append(b);
n.setData("x", 3.0);
n.evaluate();
console.log(n.getData("y"));

o.setSourceCode("operator entry( io Scalar x, io Scalar y ) { y = x * x * x; }");
n.setData("x", 2.0);
n.evaluate();
console.log(n.getData("y"));

F.close();
