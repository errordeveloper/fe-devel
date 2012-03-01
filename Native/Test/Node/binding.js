/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

b = F.DG.createBinding();
console.log(JSON.stringify(b.getOperator()));
console.log(JSON.stringify(b.getParameterLayout()));

o = F.DG.createOperator("op");
b.setOperator(o);
console.log(JSON.stringify(b.getOperator().getName()));

b.setParameterLayout(["self.foo"]);
console.log(JSON.stringify(b.getParameterLayout()));

F.close();
