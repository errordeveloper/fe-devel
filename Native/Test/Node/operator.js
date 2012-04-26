/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

o = F.DG.createOperator("op");
o.setSourceCode("operator entry() { report('Hello'); }");
o.setEntryPoint("entry");
console.log(o.getSourceCode());
console.log(o.getEntryPoint());

F.close();
