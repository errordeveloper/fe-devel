/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require("Fabric").createClient();
cv = FC.MR.createConstValue('String', 'Hello');
vmo = FC.KLC.createValueMapOperator("vm.kl", "operator vm( String input, io String output ) { output = input + ', world!'; }", "vm");
vm = FC.MR.createValueMap(cv, vmo);
console.log(vm.produce());
FC.close();
