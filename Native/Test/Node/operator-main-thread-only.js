/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

o = F.DG.createOperator("op");
o.setSourceCode("operator entry( io Integer foo ) { report 'Hello'; }");
o.setEntryFunctionName("entry");
console.log(o.getMainThreadOnly());
o.setMainThreadOnly(true);
console.log(o.getMainThreadOnly());

b = F.DG.createBinding();
b.setOperator(o);
b.setParameterLayout(["self.foo"]);

n = F.DG.createNode("foo");
n.addMember("foo", "Integer");
n.resize(64);
n.bindings.append(b);
n.evaluate();

F.close();
