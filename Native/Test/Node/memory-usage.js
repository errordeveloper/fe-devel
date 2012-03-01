/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

n = F.DG.createNode("foo");
n.addMember("bar", "String[]");
console.log(F.getMemoryUsage());

F.close();
